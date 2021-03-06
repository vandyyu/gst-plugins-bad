/* GStreamer
 *
 * unit test for logoinsert
 *
 * Copyright (C) 2011 David Schleef <ds@schleef.org>
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

#include <gst/check/gstcheck.h>
#include <string.h>

static guint8 image_data[];
/* sizeof on a forward declaration?  not so fast */
#define SIZEOF_IMAGE_DATA 1428

typedef struct
{
  GMainLoop *loop;
  gboolean eos;
} OnMessageUserData;

static void
on_message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
  OnMessageUserData *d = user_data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING:
      g_assert_not_reached ();
      break;
    case GST_MESSAGE_EOS:
      g_main_loop_quit (d->loop);
      d->eos = TRUE;
      break;
    default:
      break;
  }
}

static void
run_test (const gchar * pipeline_string)
{
  GstElement *pipeline;
  GstBus *bus;
  GMainLoop *loop;
  OnMessageUserData omud = { NULL, };
  GstStateChangeReturn ret;
  GstElement *e;
  GstBuffer *buffer;

  GST_DEBUG ("Testing pipeline '%s'", pipeline_string);

  pipeline = gst_parse_launch (pipeline_string, NULL);
  fail_unless (pipeline != NULL);
  g_object_set (G_OBJECT (pipeline), "async-handling", TRUE, NULL);

  e = gst_bin_get_by_name (GST_BIN (pipeline), "e");
  fail_unless (e != NULL);
  buffer = gst_buffer_new ();
  GST_BUFFER_DATA (buffer) = image_data;
  GST_BUFFER_SIZE (buffer) = SIZEOF_IMAGE_DATA;
  g_object_set (e, "data", buffer, NULL);
  g_object_unref (e);

  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_element_get_bus (pipeline);
  fail_unless (bus != NULL);
  gst_bus_add_signal_watch (bus);

  omud.loop = loop;
  omud.eos = FALSE;

  g_signal_connect (bus, "message", (GCallback) on_message_cb, &omud);

  gst_object_unref (bus);

  ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  fail_unless (ret == GST_STATE_CHANGE_SUCCESS
      || ret == GST_STATE_CHANGE_ASYNC);

  g_main_loop_run (loop);

  fail_unless (gst_element_set_state (pipeline,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);

  fail_unless (omud.eos == TRUE);

  gst_object_unref (pipeline);
  g_main_loop_unref (loop);
}

GST_START_TEST (test_logoinsert)
{

  run_test ("videotestsrc num-buffers=250 ! coglogoinsert name=e ! fakesink");
}

GST_END_TEST;

static Suite *
logoinsert_suite (void)
{
  Suite *s = suite_create ("logoinsert");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_set_timeout (tc_chain, 180);

  tcase_add_test (tc_chain, test_logoinsert);

  return s;
}

GST_CHECK_MAIN (logoinsert);

static guint8 image_data[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49,
  0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x08,
  0x06, 0x00, 0x00, 0x00, 0x70, 0xe2, 0x95, 0x54, 0x00, 0x00, 0x00, 0x01,
  0x73, 0x52, 0x47, 0x42, 0x00, 0xae, 0xce, 0x1c, 0xe9, 0x00, 0x00, 0x00,
  0x06, 0x62, 0x4b, 0x47, 0x44, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0xa0,
  0xbd, 0xa7, 0x93, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00,
  0x00, 0x0b, 0x13, 0x00, 0x00, 0x0b, 0x13, 0x01, 0x00, 0x9a, 0x9c, 0x18,
  0x00, 0x00, 0x00, 0x07, 0x74, 0x49, 0x4d, 0x45, 0x07, 0xdb, 0x03, 0x12,
  0x01, 0x3b, 0x19, 0x50, 0x86, 0x8e, 0x47, 0x00, 0x00, 0x00, 0x19, 0x74,
  0x45, 0x58, 0x74, 0x43, 0x6f, 0x6d, 0x6d, 0x65, 0x6e, 0x74, 0x00, 0x43,
  0x72, 0x65, 0x61, 0x74, 0x65, 0x64, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20,
  0x47, 0x49, 0x4d, 0x50, 0x57, 0x81, 0x0e, 0x17, 0x00, 0x00, 0x04, 0xef,
  0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0xed, 0x9d, 0xcf, 0x6b, 0x1d, 0x55,
  0x14, 0xc7, 0x3f, 0xf7, 0xa1, 0x89, 0x81, 0x16, 0xa3, 0x82, 0x9a, 0x57,
  0xc4, 0x59, 0x68, 0x05, 0x05, 0x13, 0x28, 0x08, 0xba, 0x49, 0x16, 0xa2,
  0xb8, 0x90, 0x06, 0xd4, 0x45, 0x75, 0x61, 0x17, 0xfe, 0x07, 0xe2, 0xca,
  0x85, 0x8a, 0x9b, 0x82, 0x14, 0x04, 0x37, 0x5d, 0x5a, 0xfc, 0x51, 0x10,
  0xd4, 0xd6, 0xba, 0x69, 0x11, 0xac, 0x22, 0x55, 0xb0, 0xd1, 0x44, 0xd0,
  0x62, 0x57, 0x53, 0x82, 0x81, 0x46, 0x6b, 0x5f, 0xd3, 0x17, 0x34, 0x21,
  0xf5, 0xb8, 0x98, 0x33, 0xed, 0x04, 0x5e, 0x9a, 0xbe, 0xf7, 0x66, 0xee,
  0xdc, 0x99, 0x39, 0x5f, 0xb8, 0x0c, 0x59, 0xcd, 0xdc, 0xf3, 0xc9, 0xf7,
  0xdc, 0x3b, 0x77, 0xde, 0x3d, 0xd7, 0x89, 0x08, 0xa6, 0x70, 0xd4, 0xb2,
  0x10, 0x18, 0x10, 0x93, 0x01, 0x31, 0x20, 0x26, 0x03, 0x62, 0x40, 0x4c,
  0x06, 0xa4, 0xfe, 0xba, 0xa5, 0xea, 0x1d, 0x70, 0xce, 0x8d, 0x03, 0xd9,
  0x96, 0x55, 0x27, 0x6d, 0x22, 0xd2, 0xa9, 0x44, 0x7f, 0x42, 0x7f, 0x31,
  0x74, 0xce, 0x8d, 0x00, 0x6d, 0x20, 0xbd, 0x02, 0xec, 0x00, 0x1e, 0x00,
  0x22, 0x60, 0xa7, 0xfe, 0x3d, 0x72, 0x23, 0x20, 0x40, 0x0c, 0x2c, 0x00,
  0xf3, 0x21, 0xc3, 0x09, 0x12, 0x88, 0x42, 0xd8, 0x0d, 0x3c, 0xa1, 0x41,
  0x9f, 0xd0, 0xc0, 0xdf, 0xaf, 0xc1, 0xcf, 0xb6, 0x9b, 0x75, 0x79, 0x07,
  0x98, 0xd7, 0xb6, 0xa0, 0xd7, 0x38, 0x34, 0x38, 0xc1, 0x00, 0x51, 0x08,
  0x51, 0x06, 0xc4, 0x6e, 0xe0, 0xf1, 0x8c, 0x2b, 0xf2, 0xd4, 0x45, 0x60,
  0x11, 0x38, 0x03, 0x7c, 0x01, 0xcc, 0x89, 0xc8, 0x92, 0x01, 0xb9, 0x0e,
  0x63, 0x07, 0xf0, 0x34, 0xf0, 0x9c, 0x82, 0xd8, 0xe3, 0xe9, 0xd6, 0x1b,
  0x40, 0x17, 0xb8, 0x04, 0x9c, 0x03, 0x3e, 0x07, 0x8e, 0x97, 0x09, 0xa7,
  0x54, 0x20, 0x99, 0xd4, 0xf4, 0x14, 0xf0, 0xa2, 0x47, 0x10, 0xbd, 0x74,
  0x55, 0x9d, 0xb3, 0x0c, 0x7c, 0x0f, 0x1c, 0x2f, 0xc3, 0x39, 0xa5, 0x01,
  0xc9, 0xb8, 0x62, 0x5f, 0x81, 0xa9, 0x69, 0x50, 0x75, 0x15, 0xce, 0x29,
  0xe0, 0x3d, 0x11, 0x99, 0xab, 0x35, 0x10, 0x85, 0xf1, 0x2c, 0xf0, 0x6a,
  0xc9, 0xae, 0xd8, 0x4e, 0xab, 0x3a, 0xce, 0x1c, 0xf1, 0x96, 0xca, 0x44,
  0xc4, 0x6b, 0xd3, 0x99, 0xd1, 0x3e, 0xed, 0xa8, 0x54, 0xa0, 0x6d, 0x00,
  0x7f, 0x01, 0x1f, 0x03, 0x7b, 0x0a, 0x8f, 0x4f, 0x49, 0x30, 0x7e, 0xae,
  0x08, 0x8c, 0x6c, 0x5b, 0x07, 0x7e, 0xd0, 0xe7, 0x1f, 0xaf, 0x3c, 0x10,
  0x7d, 0x71, 0x9b, 0x06, 0x7e, 0xa9, 0x20, 0x8c, 0xb4, 0x5d, 0xd5, 0xe9,
  0xf2, 0x41, 0x20, 0xaa, 0x3a, 0x90, 0x87, 0x74, 0xce, 0x2f, 0x35, 0x68,
  0x5d, 0xe0, 0x23, 0x60, 0x26, 0x6f, 0xb7, 0x78, 0x19, 0xd4, 0x9d, 0x73,
  0x0f, 0x03, 0xaf, 0xeb, 0x7b, 0xc6, 0x68, 0x4d, 0xd6, 0x01, 0xd7, 0x80,
  0x9f, 0x80, 0x4f, 0x80, 0xa3, 0x22, 0x12, 0x57, 0x62, 0x71, 0xd1, 0x39,
  0xf7, 0x20, 0xf0, 0x1a, 0xf0, 0xbc, 0xa6, 0xad, 0xba, 0x68, 0x54, 0xa7,
  0xeb, 0x13, 0xc0, 0xb8, 0x73, 0xee, 0xfd, 0x3c, 0xa0, 0xb4, 0x0a, 0x86,
  0x11, 0x01, 0xaf, 0x00, 0xb3, 0x35, 0x83, 0x91, 0x55, 0x04, 0xbc, 0x0c,
  0xec, 0xd7, 0xfe, 0x86, 0x09, 0x44, 0x1f, 0x6e, 0x3f, 0xf0, 0x52, 0x8f,
  0x55, 0x58, 0x83, 0xe2, 0x13, 0x88, 0x7e, 0xa3, 0x98, 0x55, 0x20, 0xbb,
  0x68, 0x86, 0x52, 0x28, 0xb3, 0xda, 0xff, 0xa0, 0x1c, 0x32, 0x05, 0xec,
  0x25, 0x59, 0x2e, 0x6f, 0x92, 0x22, 0xed, 0xf7, 0x54, 0x30, 0x40, 0xd4,
  0xb2, 0x43, 0x3d, 0x54, 0xc5, 0x35, 0x05, 0xec, 0x1d, 0x34, 0x75, 0xb5,
  0x72, 0x86, 0x91, 0xa6, 0xaa, 0xd9, 0x06, 0x8c, 0x1b, 0x5b, 0xe9, 0x5a,
  0x0c, 0x06, 0x49, 0x5d, 0xad, 0x02, 0x2c, 0x3b, 0xad, 0xd7, 0x26, 0x6b,
  0xe0, 0x38, 0xb4, 0x0a, 0xb0, 0xeb, 0x14, 0xa6, 0x81, 0x63, 0x91, 0x1b,
  0x10, 0xb5, 0xe7, 0xa4, 0xb9, 0x63, 0x93, 0x4b, 0x26, 0xfb, 0x4d, 0x5b,
  0x2d, 0x73, 0x47, 0x58, 0x2e, 0xc9, 0x13, 0xc8, 0xa4, 0x01, 0xe9, 0x09,
  0x64, 0xd2, 0x3b, 0x10, 0xb5, 0x65, 0xd4, 0xe0, 0x99, 0xd5, 0x8d, 0x66,
  0x5c, 0x51, 0x3f, 0x69, 0xab, 0x95, 0xe3, 0x8d, 0xdb, 0x16, 0xff, 0x2d,
  0x63, 0x53, 0x0a, 0x90, 0x7b, 0x2d, 0xf6, 0x61, 0x01, 0xd9, 0x65, 0xb1,
  0x0f, 0x07, 0xc8, 0x5d, 0xc0, 0x98, 0xc5, 0x7e, 0x78, 0xe5, 0x05, 0xe4,
  0x11, 0xe0, 0x76, 0x0b, 0x67, 0x38, 0x40, 0xee, 0x33, 0x87, 0x04, 0x02,
  0xc4, 0x39, 0xd7, 0x06, 0xee, 0xc0, 0x36, 0xff, 0x6c, 0xa5, 0x74, 0x3b,
  0x84, 0x37, 0x87, 0x8c, 0x90, 0xfc, 0x12, 0xc3, 0x14, 0x08, 0x90, 0x36,
  0x70, 0xa7, 0xc5, 0x3d, 0x2c, 0x87, 0x58, 0xba, 0xea, 0xad, 0x35, 0xe0,
  0x42, 0x3f, 0x9b, 0x82, 0x2c, 0x90, 0xc5, 0xea, 0x1f, 0xe0, 0xdf, 0x32,
  0x66, 0x59, 0xa6, 0xde, 0x1a, 0x03, 0x6e, 0x33, 0x20, 0xe1, 0x68, 0x14,
  0xb8, 0xc7, 0xf7, 0xe2, 0xe2, 0x52, 0x3f, 0x83, 0x56, 0x03, 0xe5, 0x7d,
  0xe9, 0x64, 0x9d, 0x64, 0x0f, 0x85, 0x29, 0x10, 0x20, 0x4b, 0x3a, 0x78,
  0x99, 0x42, 0x00, 0x22, 0x22, 0xeb, 0x24, 0x1b, 0x25, 0xd7, 0x2c, 0xf6,
  0xc3, 0x2b, 0xaf, 0x41, 0xfd, 0x57, 0x60, 0xc5, 0xc2, 0x19, 0x0e, 0x90,
  0x45, 0x4b, 0x5b, 0x61, 0x01, 0xd9, 0x00, 0x2e, 0x5b, 0x38, 0x7b, 0xca,
  0xfb, 0xd2, 0x49, 0x3a, 0xb0, 0x9f, 0xb7, 0xd8, 0x87, 0x03, 0x64, 0x1d,
  0xf8, 0xdb, 0x62, 0x1f, 0x96, 0x43, 0xfe, 0xb0, 0xd8, 0xf7, 0x84, 0x11,
  0x7b, 0x5f, 0x5c, 0xd4, 0xa9, 0x6f, 0xac, 0x60, 0x4c, 0xd7, 0x95, 0x96,
  0x82, 0xf2, 0x3e, 0xa8, 0x03, 0x9c, 0x26, 0x29, 0xda, 0x62, 0xda, 0x0c,
  0x64, 0xbe, 0x2c, 0x20, 0xe7, 0xb4, 0x99, 0x12, 0xc5, 0xc0, 0x42, 0xbf,
  0x05, 0xd2, 0x72, 0x03, 0xa2, 0x69, 0xeb, 0x34, 0x30, 0x67, 0x2c, 0x06,
  0x73, 0x47, 0xde, 0x0e, 0x31, 0x97, 0x6c, 0x76, 0xc7, 0x37, 0x7a, 0x2d,
  0x15, 0x48, 0x0c, 0x7c, 0xda, 0x70, 0x97, 0x74, 0x80, 0xa3, 0x24, 0xd5,
  0x1d, 0xfa, 0xfe, 0x2c, 0x91, 0x6b, 0x25, 0x07, 0x11, 0x59, 0x77, 0xce,
  0x9d, 0x20, 0xd9, 0x7d, 0x3b, 0x41, 0x33, 0x7f, 0x80, 0x3d, 0x0f, 0x1c,
  0x1b, 0xb4, 0xaa, 0x43, 0xee, 0x5f, 0x0c, 0x45, 0xa4, 0x0b, 0x9c, 0x6c,
  0xe8, 0x8c, 0x2b, 0x06, 0x8e, 0x0d, 0x32, 0x76, 0x14, 0x06, 0x24, 0x33,
  0x96, 0x1c, 0x69, 0x58, 0xea, 0x5a, 0x04, 0x0e, 0x0f, 0x9a, 0xaa, 0x0a,
  0x05, 0xa2, 0x33, 0xae, 0x13, 0x24, 0x75, 0xa5, 0x9a, 0x00, 0xe5, 0x4f,
  0xe0, 0x03, 0x60, 0xf8, 0x02, 0x34, 0x9e, 0x2a, 0xc8, 0x55, 0xb9, 0x68,
  0xd9, 0x76, 0xed, 0x12, 0xf0, 0x0e, 0x39, 0x15, 0x34, 0xf3, 0x59, 0xd6,
  0xef, 0xf7, 0x1a, 0xc2, 0xb8, 0x08, 0xbc, 0x4d, 0x8e, 0xd5, 0xe5, 0x7c,
  0xd6, 0x5a, 0x9c, 0x06, 0x7e, 0xac, 0x11, 0x8c, 0x55, 0xe0, 0x2d, 0xa0,
  0x5d, 0xe5, 0x9a, 0x8b, 0x8f, 0x01, 0x67, 0x6b, 0x00, 0xe3, 0x0a, 0xf0,
  0x2e, 0x70, 0x77, 0xa5, 0xab, 0x92, 0x2a, 0x98, 0x67, 0xa8, 0x66, 0x55,
  0xd2, 0x6c, 0x9a, 0x3a, 0x50, 0x04, 0x8c, 0x52, 0x80, 0x28, 0x94, 0x27,
  0x81, 0xef, 0x2a, 0x08, 0xe3, 0x02, 0xf0, 0x66, 0x51, 0x30, 0xbc, 0x15,
  0xc1, 0xec, 0x25, 0xe7, 0xdc, 0x0c, 0xf0, 0x06, 0x49, 0x65, 0xcf, 0xd0,
  0xb5, 0xaa, 0x30, 0x0e, 0x03, 0x87, 0x44, 0x64, 0xb9, 0xa8, 0x1b, 0x95,
  0x76, 0xc2, 0x8e, 0x88, 0x9c, 0x72, 0xce, 0xa5, 0x6f, 0xb7, 0x33, 0x84,
  0x5b, 0x23, 0x65, 0x19, 0xf8, 0x5a, 0x61, 0x7c, 0x2b, 0x22, 0xab, 0x85,
  0xfe, 0xa3, 0x96, 0x7d, 0x5c, 0x85, 0x16, 0xfa, 0x9a, 0x26, 0x29, 0x07,
  0x38, 0x13, 0x18, 0x88, 0x15, 0x7d, 0xe1, 0x2b, 0xd4, 0x15, 0x41, 0x38,
  0x24, 0xe3, 0x94, 0x18, 0x88, 0x9d, 0x73, 0xe7, 0xd5, 0x2d, 0x51, 0x89,
  0x60, 0xd2, 0xd4, 0xb4, 0x02, 0x7c, 0xa6, 0x4b, 0xe8, 0x73, 0x45, 0xbb,
  0x22, 0x28, 0x87, 0xf4, 0x70, 0x4b, 0xea, 0x98, 0xc8, 0x43, 0x2a, 0xfb,
  0x4f, 0x67, 0x4d, 0x1d, 0x85, 0xf1, 0x1b, 0xf0, 0xa1, 0x8f, 0xd4, 0x54,
  0x09, 0x20, 0x5b, 0xa4, 0xb2, 0x14, 0xd2, 0xad, 0xc0, 0xa3, 0x24, 0x45,
  0x0a, 0x86, 0x5d, 0xd6, 0xbf, 0x0c, 0x7c, 0xa5, 0xab, 0x07, 0x27, 0x15,
  0xc6, 0xd9, 0xb2, 0x20, 0x54, 0x02, 0x48, 0x0f, 0x38, 0x64, 0xdc, 0x32,
  0x46, 0x72, 0x4e, 0xd5, 0x0b, 0x24, 0x35, 0xe5, 0x6f, 0x46, 0x57, 0x80,
  0x2f, 0x81, 0x83, 0x3e, 0x0f, 0x68, 0xa9, 0x25, 0x90, 0x6d, 0x60, 0xed,
  0x24, 0x39, 0x14, 0x26, 0xbd, 0x66, 0xd5, 0x25, 0x39, 0xa7, 0xa4, 0x2b,
  0x22, 0x67, 0x2a, 0xd1, 0x1f, 0x3b, 0xe0, 0x3e, 0x2c, 0xd9, 0x1e, 0x43,
  0x03, 0x62, 0x32, 0x20, 0x06, 0xc4, 0x64, 0x40, 0x0c, 0x88, 0xc9, 0x80,
  0x18, 0x10, 0x93, 0x01, 0x31, 0x20, 0x26, 0x03, 0x62, 0x32, 0x20, 0x06,
  0xc4, 0x64, 0x40, 0x0c, 0x88, 0xc9, 0x80, 0x18, 0x10, 0x93, 0x01, 0x69,
  0xb2, 0xfe, 0x07, 0xb0, 0x6d, 0xc6, 0x8c, 0x41, 0x92, 0x78, 0x97, 0x00,
  0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};
