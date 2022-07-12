/*
 * Copyright (C) 2020 ~ 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     WangZhengYang <wangzhengyang@uniontech.com>
 *
 * Maintainer: HouChengQiu <houchengqiu@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//移植自gst/gst.h头文件

#pragma once

#include <glib-object.h>
#include <stdint.h>

#if 1

//基础类型定义
struct GstElement;
struct GstCaps;
struct GstSample;
struct GstBus;
struct GstBuffer;
struct GstStructure;
struct GstMessage;
struct GstBin;
struct GstMemory;
struct GstObject;
struct GstTagList;
using gpointer = void*;
using GstClockTime = guint64;
using GstClockTimeDiff = gint64;

typedef enum {
  /* one of these */
  GST_SEEK_TYPE_NONE            = 0,
  GST_SEEK_TYPE_SET             = 1,
  GST_SEEK_TYPE_END             = 2
} GstSeekType;

typedef enum {
  /* custom success starts here */
  GST_FLOW_CUSTOM_SUCCESS_2 = 102,
  GST_FLOW_CUSTOM_SUCCESS_1 = 101,
  GST_FLOW_CUSTOM_SUCCESS = 100,

  /* core predefined */
  GST_FLOW_OK		  =  0,
  /* expected failures */
  GST_FLOW_NOT_LINKED     = -1,
  GST_FLOW_FLUSHING       = -2,
  /* error cases */
  GST_FLOW_EOS            = -3,
  GST_FLOW_NOT_NEGOTIATED = -4,
  GST_FLOW_ERROR	  = -5,
  GST_FLOW_NOT_SUPPORTED  = -6,

  /* custom error starts here */
  GST_FLOW_CUSTOM_ERROR   = -100,
  GST_FLOW_CUSTOM_ERROR_1 = -101,
  GST_FLOW_CUSTOM_ERROR_2 = -102
} GstFlowReturn;

typedef enum {
  GST_LOCK_FLAG_READ      = (1 << 0),
  GST_LOCK_FLAG_WRITE     = (1 << 1),
  GST_LOCK_FLAG_EXCLUSIVE = (1 << 2),

  GST_LOCK_FLAG_LAST      = (1 << 8)
} GstLockFlags;

typedef enum {
  GST_MAP_READ      = GST_LOCK_FLAG_READ,
  GST_MAP_WRITE     = GST_LOCK_FLAG_WRITE,

  GST_MAP_FLAG_LAST = (1 << 16)
} GstMapFlags;

typedef struct {
  GstMemory *memory;
  GstMapFlags flags;
  guint8 *data;
  gsize size;
  gsize maxsize;
  /*< protected >*/
  gpointer user_data[4];

  /*< private >*/
  gpointer _gst_reserved[4];
} GstMapInfo;

typedef enum
{
  GST_MESSAGE_UNKNOWN           = 0,
  GST_MESSAGE_EOS               = (1 << 0),
  GST_MESSAGE_ERROR             = (1 << 1),
  GST_MESSAGE_WARNING           = (1 << 2),
  GST_MESSAGE_INFO              = (1 << 3),
  GST_MESSAGE_TAG               = (1 << 4),
  GST_MESSAGE_BUFFERING         = (1 << 5),
  GST_MESSAGE_STATE_CHANGED     = (1 << 6),
  GST_MESSAGE_STATE_DIRTY       = (1 << 7),
  GST_MESSAGE_STEP_DONE         = (1 << 8),
  GST_MESSAGE_CLOCK_PROVIDE     = (1 << 9),
  GST_MESSAGE_CLOCK_LOST        = (1 << 10),
  GST_MESSAGE_NEW_CLOCK         = (1 << 11),
  GST_MESSAGE_STRUCTURE_CHANGE  = (1 << 12),
  GST_MESSAGE_STREAM_STATUS     = (1 << 13),
  GST_MESSAGE_APPLICATION       = (1 << 14),
  GST_MESSAGE_ELEMENT           = (1 << 15),
  GST_MESSAGE_SEGMENT_START     = (1 << 16),
  GST_MESSAGE_SEGMENT_DONE      = (1 << 17),
  GST_MESSAGE_DURATION_CHANGED  = (1 << 18),
  GST_MESSAGE_LATENCY           = (1 << 19),
  GST_MESSAGE_ASYNC_START       = (1 << 20),
  GST_MESSAGE_ASYNC_DONE        = (1 << 21),
  GST_MESSAGE_REQUEST_STATE     = (1 << 22),
  GST_MESSAGE_STEP_START        = (1 << 23),
  GST_MESSAGE_QOS               = (1 << 24),
  GST_MESSAGE_PROGRESS          = (1 << 25),
  GST_MESSAGE_TOC               = (1 << 26),
  GST_MESSAGE_RESET_TIME        = (1 << 27),
  GST_MESSAGE_STREAM_START      = (1 << 28),
  GST_MESSAGE_NEED_CONTEXT      = (1 << 29),
  GST_MESSAGE_HAVE_CONTEXT      = (1 << 30),
  GST_MESSAGE_EXTENDED          = (gint) (1u << 31),
  GST_MESSAGE_DEVICE_ADDED      = GST_MESSAGE_EXTENDED + 1,
  GST_MESSAGE_DEVICE_REMOVED    = GST_MESSAGE_EXTENDED + 2,
  GST_MESSAGE_PROPERTY_NOTIFY   = GST_MESSAGE_EXTENDED + 3,
  GST_MESSAGE_STREAM_COLLECTION = GST_MESSAGE_EXTENDED + 4,
  GST_MESSAGE_STREAMS_SELECTED  = GST_MESSAGE_EXTENDED + 5,
  GST_MESSAGE_REDIRECT          = GST_MESSAGE_EXTENDED + 6,
  GST_MESSAGE_ANY               = (gint) (0xffffffff)
} GstMessageType;

typedef enum {
  GST_FORMAT_UNDEFINED  =  0, /* must be first in list */
  GST_FORMAT_DEFAULT    =  1,
  GST_FORMAT_BYTES      =  2,
  GST_FORMAT_TIME       =  3,
  GST_FORMAT_BUFFERS    =  4,
  GST_FORMAT_PERCENT    =  5
} GstFormat;

typedef enum {
  GST_SEEK_FLAG_NONE            = 0,
  GST_SEEK_FLAG_FLUSH           = (1 << 0),
  GST_SEEK_FLAG_ACCURATE        = (1 << 1),
  GST_SEEK_FLAG_KEY_UNIT        = (1 << 2),
  GST_SEEK_FLAG_SEGMENT         = (1 << 3),
  GST_SEEK_FLAG_TRICKMODE       = (1 << 4),
  /* FIXME 2.0: Remove _SKIP flag,
   * which was kept for backward compat when _TRICKMODE was added */
  GST_SEEK_FLAG_SKIP            = (1 << 4),
  GST_SEEK_FLAG_SNAP_BEFORE     = (1 << 5),
  GST_SEEK_FLAG_SNAP_AFTER      = (1 << 6),
  GST_SEEK_FLAG_SNAP_NEAREST    = GST_SEEK_FLAG_SNAP_BEFORE | GST_SEEK_FLAG_SNAP_AFTER,
  /* Careful to restart next flag with 1<<7 here */
  GST_SEEK_FLAG_TRICKMODE_KEY_UNITS = (1 << 7),
  GST_SEEK_FLAG_TRICKMODE_NO_AUDIO  = (1 << 8),
} GstSeekFlags;

struct GstMiniObject {
  GType   type;

  /*< public >*/ /* with COW */
  gint    refcount;
  gint    lockstate;
  guint   flags;

  GstMiniObject* (*copy)(const GstMiniObject *obj);
  gboolean (*dispose)(GstMiniObject *obj);
  void (*free)(GstMiniObject *obj);

  /* < private > */
  /* Used to keep track of weak ref notifies and qdata */
  guint n_qdata;
  gpointer qdata;
};

struct GstMessage
{
  GstMiniObject   mini_object;

  /*< public > *//* with COW */
  GstMessageType  type;
  guint64         timestamp;
  GstObject      *src;
  guint32         seqnum;

  /*< private >*//* with MESSAGE_LOCK */
  GMutex          lock;                 /* lock and cond for async delivery */
  GCond           cond;
};

typedef enum {
  GST_STATE_CHANGE_FAILURE             = 0,
  GST_STATE_CHANGE_SUCCESS             = 1,
  GST_STATE_CHANGE_ASYNC               = 2,
  GST_STATE_CHANGE_NO_PREROLL          = 3
} GstStateChangeReturn;

typedef enum {
  GST_STATE_VOID_PENDING        = 0,
  GST_STATE_NULL                = 1,
  GST_STATE_READY               = 2,
  GST_STATE_PAUSED              = 3,
  GST_STATE_PLAYING             = 4
} GstState;

//GST基础函数
/*guint gst_caps_get_size(const GstCaps *caps);
GstStructure* gst_caps_get_structure(const GstCaps *caps, guint index);
gboolean gst_structure_has_name(const GstStructure* structure, const gchar* name);
GstElement* gst_element_factory_make(const gchar *factoryname, const gchar *name);
void gst_object_unref(gpointer object);
GstElement* gst_pipeline_new(const gchar *name);
void gst_bin_add_many(GstBin *bin, GstElement *element_1, ...) G_GNUC_NULL_TERMINATED;
gboolean gst_element_link_many(GstElement *element_1, GstElement *element_2, ...);
GType gst_bin_get_type(void);
GstBuffer* gst_sample_get_buffer(GstSample *sample);
GstCaps* gst_sample_get_caps(GstSample *sample);
GstCaps* gst_caps_new_empty(void);
GstStructure* gst_structure_copy(const GstStructure  * structure);
void gst_structure_remove_field(GstStructure* structure, const gchar* fieldname);
void gst_caps_append_structure(GstCaps *caps, GstStructure  *structure);
GstStateChangeReturn gst_element_set_state(GstElement *element, GstState state);
void gst_mini_object_unref(GstMiniObject *mini_object);
GstBus* gst_element_get_bus(GstElement * element);
GstMessage* gst_bus_timed_pop_filtered(GstBus * bus, GstClockTime timeout, GstMessageType types);
void gst_message_parse_error(GstMessage *message, GError **gerror, gchar **debug);
GstElement* gst_bin_get_by_name(GstBin *bin, const gchar *name);
gboolean gst_structure_get_int(const GstStructure* structure, const gchar* fieldname, gint* value);
GstMemory* gst_buffer_get_memory(GstBuffer *buffer, guint idx);
void gst_memory_unmap(GstMemory *mem, GstMapInfo *info);
gboolean gst_element_seek(GstElement *element, gdouble rate, GstFormat format, GstSeekFlags flags, GstSeekType start_type, gint64 start, GstSeekType stop_type, gint64 stop);
GstStateChangeReturn gst_element_get_state(GstElement* element, GstState* state, GstState* pending, GstClockTime timeout);
gboolean gst_element_query_duration(GstElement* element, GstFormat format, gint64* duration);
void gst_init(int *argc, char **argv[]);
gboolean gst_memory_map(GstMemory *mem, GstMapInfo *info, GstMapFlags flags);
gboolean gst_tag_list_get_string_index(const GstTagList* list, const gchar* tag, guint index, gchar** value);
GstCaps* gst_caps_new_simple(const char* media_type, const char* fieldname, ...);
GType _gst_fraction_type; //大作用域变量, resolve可以解出*/

//GST宏函数
#define GST_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), gst_bin_get_type(), GstBin))
#define GST_MESSAGE_CAST(obj) ((GstMessage*)(obj))
#define GST_MESSAGE_TYPE(message) (GST_MESSAGE_CAST(message)->type)
#define GST_MINI_OBJECT_CAST(obj) (reinterpret_cast<GstMiniObject*>(obj))
#define GST_SECOND  ((GstClockTimeDiff)(G_USEC_PER_SEC * G_GINT64_CONSTANT (1000)))
#define GST_MSECOND ((GstClockTimeDiff)(GST_SECOND / G_GINT64_CONSTANT (1000)))
#define GST_CLOCK_TIME_NONE ((GstClockTime) -1)
#define GST_MESSAGE_SRC(message) (GST_MESSAGE_CAST(message)->src)

#else

#include <gst/gst.h>

#endif
