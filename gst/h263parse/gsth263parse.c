/* GStreamer H.263 Parser
 * Copyright (C) <2010> Arun Raghavan <arun.raghavan@collabora.co.uk>
 * Copyright (C) <2010> Edward Hervey <edward.hervey@collabora.co.uk>
 * Copyright (C) <2010> Collabora Multimedia
 * Copyright (C) <2010> Nokia Corporation
 *
 * Some bits C-c,C-v'ed and s/4/3 from h264parse:
 *           (C) 2005 Michal Benes <michal.benes@itonis.tv>
 *           (C) 2008 Wim Taymans <wim.taymans@gmail.com>
 *           (C) 2009 Mark Nauwelaerts <mnauw users sf net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/base/gstbytereader.h>
#include "gsth263parse.h"

GST_DEBUG_CATEGORY (h263_parse_debug);

static GstStaticPadTemplate srctemplate =
GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h263, variant = (string) itu, "
        "parsed = (boolean) true")
    );

static GstStaticPadTemplate sinktemplate =
GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h263, variant = (string) itu, "
        "parsed = (boolean) false")
    );

GST_BOILERPLATE (GstH263Parse, gst_h263_parse, GstElement, GST_TYPE_BASE_PARSE);

static gboolean gst_h263_parse_start (GstBaseParse * parse);
static gboolean gst_h263_parse_stop (GstBaseParse * parse);
static gboolean gst_h263_parse_sink_event (GstBaseParse * parse,
    GstEvent * event);
static gboolean gst_h263_parse_check_valid_frame (GstBaseParse * parse,
    GstBuffer * buffer, guint * framesize, gint * skipsize);
static GstFlowReturn gst_h263_parse_parse_frame (GstBaseParse * parse,
    GstBuffer * buffer);
static GstCaps *gst_h263_parse_src_getcaps (GstPad * pad);

static void
gst_h263_parse_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srctemplate));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sinktemplate));
  gst_element_class_set_details_simple (gstelement_class, "H.263 parser",
      "Codec/Parser/Video",
      "Parses H.263 streams",
      "Arun Raghavan <arun.raghavan@collabora.co.uk>,"
      "Edward Hervey <edward.hervey@collabora.co.uk>");

  GST_DEBUG_CATEGORY_INIT (h263_parse_debug, "h263parse", 0, "h263 parser");
}

static void
gst_h263_parse_class_init (GstH263ParseClass * klass)
{
  GstBaseParseClass *parse_class = GST_BASE_PARSE_CLASS (klass);

  /* Override BaseParse vfuncs */
  parse_class->start = GST_DEBUG_FUNCPTR (gst_h263_parse_start);
  parse_class->stop = GST_DEBUG_FUNCPTR (gst_h263_parse_stop);
  parse_class->event = GST_DEBUG_FUNCPTR (gst_h263_parse_sink_event);
  parse_class->check_valid_frame =
      GST_DEBUG_FUNCPTR (gst_h263_parse_check_valid_frame);
  parse_class->parse_frame = GST_DEBUG_FUNCPTR (gst_h263_parse_parse_frame);
}

static void
gst_h263_parse_init (GstH263Parse * h263parse, GstH263ParseClass * g_class)
{
  gst_pad_set_getcaps_function (GST_BASE_PARSE_SRC_PAD (GST_BASE_PARSE
          (h263parse)), GST_DEBUG_FUNCPTR (gst_h263_parse_src_getcaps));
}

static gboolean
gst_h263_parse_start (GstBaseParse * parse)
{
  GstH263Parse *h263parse = GST_H263PARSE (parse);

  GST_DEBUG ("Start");

  h263parse->last_pos = 0;
  h263parse->psc_pos = -1;

  h263parse->bitrate = 0;
  h263parse->profile = -1;
  h263parse->level = -1;

  h263parse->state = PARSING;

  gst_base_parse_set_passthrough (parse, FALSE);

  return TRUE;
}

static gboolean
gst_h263_parse_stop (GstBaseParse * parse)
{
  GST_DEBUG ("Stop");

  return TRUE;
}

static gboolean
gst_h263_parse_sink_event (GstBaseParse * parse, GstEvent * event)
{
  GstH263Parse *h263parse;
  gboolean res = FALSE;

  h263parse = GST_H263PARSE (parse);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:
    {
      GstTagList *taglist;

      gst_event_parse_tag (event, &taglist);

      if (gst_tag_list_get_uint (taglist, GST_TAG_BITRATE, &h263parse->bitrate))
        GST_DEBUG ("Got bitrate tag: %u", h263parse->bitrate);

      break;
    }

    default:
      break;
  }

  return res;
}

static guint
find_psc (GstBuffer * buffer, guint skip)
{
  GstByteReader br;
  guint psc_pos = -1, psc;

  gst_byte_reader_init_from_buffer (&br, buffer);

  if (!gst_byte_reader_set_pos (&br, skip))
    goto out;

  gst_byte_reader_peek_uint24_be (&br, &psc);

  /* Scan for the picture start code (22 bits - 0x0020) */
  while ((gst_byte_reader_get_remaining (&br) >= 3)) {
    if (gst_byte_reader_peek_uint24_be (&br, &psc) &&
        ((psc & 0xffffc0) == 0x000080)) {
      psc_pos = gst_byte_reader_get_pos (&br);
      break;
    } else
      gst_byte_reader_skip (&br, 1);
  }

out:
  return psc_pos;
}

static gboolean
gst_h263_parse_check_valid_frame (GstBaseParse * parse, GstBuffer * buffer,
    guint * framesize, gint * skipsize)
{
  GstH263Parse *h263parse;
  guint psc_pos;

  h263parse = GST_H263PARSE (parse);

  psc_pos = find_psc (buffer, h263parse->last_pos);

  if (psc_pos == -1) {
    /* PSC not found, need more data */
    if (GST_BUFFER_SIZE (buffer) > 3)
      h263parse->last_pos = GST_BUFFER_SIZE (buffer) - 3;
    goto more;
  }

  if (h263parse->psc_pos == -1) {
    /* Found the start of the frame, now try to find the end */

    h263parse->psc_pos = psc_pos;
    h263parse->last_pos = psc_pos + 3;

    psc_pos = find_psc (buffer, h263parse->last_pos);

    if (psc_pos == -1) {
      if (gst_base_parse_get_drain (GST_BASE_PARSE (h263parse)))
        /* FLUSH/EOS, it's okay if we can't find the next frame */
        psc_pos = GST_BUFFER_SIZE (buffer);
      else
        goto more;
    }
  }

  /* We should now have a complete frame */

  /* If this is the first frame, parse and set srcpad caps */
  if (h263parse->state == PARSING) {
    H263Params *params = NULL;
    GstFlowReturn res = gst_h263_parse_get_params (h263parse, buffer,
        &params, FALSE);

    if (res != GST_FLOW_OK || h263parse->state != GOT_HEADER) {
      GST_WARNING ("Couldn't parse header - setting passthrough mode");
      gst_base_parse_set_passthrough (parse, TRUE);
    } else {
      /* Set srcpad caps since we now have sufficient information to do so */
      gst_h263_parse_set_src_caps (h263parse, params);
    }

    if (params)
      g_free (params);
  }

  *skipsize = h263parse->psc_pos;
  *framesize = psc_pos - h263parse->psc_pos;

  h263parse->psc_pos = -1;
  h263parse->last_pos = 0;

  /* XXX: After getting a keyframe, should we adjust min_frame_size to
   * something smaller so we don't end up collecting too many non-keyframes? */

  GST_DEBUG ("Found a frame of size %d at pos %d", *framesize, *skipsize);

  return TRUE;

more:
  /* Ask for 1024 bytes more - this is an arbitrary choice */
  gst_base_parse_set_min_frame_size (parse, GST_BUFFER_SIZE (buffer) + 1024);

  /* Prevent baseparse from auto-skipping one byte */
  *skipsize = 0;

  return FALSE;
}

static GstFlowReturn
gst_h263_parse_parse_frame (GstBaseParse * parse, GstBuffer * buffer)
{
  GstH263Parse *h263parse;
  GstFlowReturn res;
  H263Params *params = NULL;

  h263parse = GST_H263PARSE (parse);

  res = gst_h263_parse_get_params (h263parse, buffer, &params, TRUE);
  if (res != GST_FLOW_OK)
    goto out;

  if (h263parse->state == PASSTHROUGH || h263parse->state == PARSING) {
    /* There's a feature we don't support, or we didn't have enough data to
     * parse the header, which should not be possible. Either way, go into
     * passthrough mode and let downstream handle it if it can. */
    GST_WARNING ("Couldn't parse header - setting passthrough mode");
    gst_base_parse_set_passthrough (parse, TRUE);
    goto out;
  }

  /* h263parse->state is now GOT_HEADER */

  gst_buffer_set_caps (buffer,
      GST_PAD_CAPS (GST_BASE_PARSE_SRC_PAD (GST_BASE_PARSE (h263parse))));

  if (gst_h263_parse_is_delta_unit (params))
    GST_BUFFER_FLAG_UNSET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);
  else
    GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);

out:
  g_free (params);
  return res;
}

static GstCaps *
gst_h263_parse_src_getcaps (GstPad * pad)
{
  GstH263Parse *h263parse;
  GstCaps *caps;

  h263parse = GST_H263PARSE (GST_PAD_PARENT (pad));

  if (h263parse->state == PARSING)
    /* Not got more specific caps from headers yet - return template caps */
    return gst_caps_copy (gst_pad_get_pad_template_caps (pad));

  g_object_get (pad, "caps", &caps, NULL);
  return caps;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "h263parse",
      GST_RANK_NONE, GST_TYPE_H263PARSE);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "h263parse",
    "Element for parsing raw h263 streams",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
