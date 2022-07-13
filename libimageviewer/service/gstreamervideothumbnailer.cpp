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
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include <QUrl>
#include <QImage>
#include <QLibrary>
#include <QLibraryInfo>
#include <QDir>

#include "gsttransplant.h"

//参考自totem项目

//基础函数指针
static guint(*gst_caps_get_size)(const GstCaps *) = nullptr;
static GstStructure*(*gst_caps_get_structure)(const GstCaps *, guint) = nullptr;
static gboolean(*gst_structure_has_name)(const GstStructure* structure, const gchar* name) = nullptr;
static GstElement* (*gst_element_factory_make)(const gchar *factoryname, const gchar *name) = nullptr;
static void (*gst_object_unref)(gpointer object) = nullptr;
static GstElement* (*gst_pipeline_new)(const gchar *name) = nullptr;
static void (*gst_bin_add_many)(GstBin *bin, GstElement *element_1, ...) = nullptr;
static gboolean (*gst_element_link_many)(GstElement *element_1, GstElement *element_2, ...) = nullptr;
static GType (*gst_bin_get_type)(void) = nullptr;
static GstBuffer* (*gst_sample_get_buffer)(GstSample *sample) = nullptr;
static GstCaps* (*gst_sample_get_caps)(GstSample *sample) = nullptr;
static GstCaps* (*gst_caps_new_empty)(void) = nullptr;
static GstStructure* (*gst_structure_copy)(const GstStructure  * structure) = nullptr;
static void (*gst_structure_remove_field)(GstStructure* structure, const gchar* fieldname) = nullptr;
static void (*gst_caps_append_structure)(GstCaps *caps, GstStructure  *structure) = nullptr;
static GstStateChangeReturn (*gst_element_set_state)(GstElement *element, GstState state) = nullptr;
static void (*gst_mini_object_unref)(GstMiniObject *mini_object) = nullptr;
static GstBus* (*gst_element_get_bus)(GstElement * element) = nullptr;
static GstMessage* (*gst_bus_timed_pop_filtered)(GstBus * bus, GstClockTime timeout, GstMessageType types) = nullptr;
static void (*gst_message_parse_error)(GstMessage *message, GError **gerror, gchar **debug) = nullptr;
static GstElement* (*gst_bin_get_by_name)(GstBin *bin, const gchar *name) = nullptr;
static gboolean (*gst_structure_get_int)(const GstStructure* structure, const gchar* fieldname, gint* value) = nullptr;
static GstMemory* (*gst_buffer_get_memory)(GstBuffer *buffer, guint idx) = nullptr;
static void (*gst_memory_unmap)(GstMemory *mem, GstMapInfo *info) = nullptr;
static gboolean (*gst_element_seek)(GstElement *element, gdouble rate, GstFormat format, GstSeekFlags flags, GstSeekType start_type, gint64 start, GstSeekType stop_type, gint64 stop) = nullptr;
static GstStateChangeReturn (*gst_element_get_state)(GstElement* element, GstState* state, GstState* pending, GstClockTime timeout) = nullptr;
static gboolean (*gst_element_query_duration)(GstElement* element, GstFormat format, gint64* duration) = nullptr;
static void (*gst_init)(int *argc, char **argv[]) = nullptr;
static gboolean (*gst_memory_map)(GstMemory *mem, GstMapInfo *info, GstMapFlags flags) = nullptr;
static gboolean (*gst_tag_list_get_string_index)(const GstTagList* list, const gchar* tag, guint index, gchar** value) = nullptr;
static GstCaps* (*gst_caps_new_simple)(const char* media_type, const char* fieldname, ...) = nullptr;

//大作用域变量 潜在崩溃风险,需要多观察
static GType* _gst_fraction_type;

//解析成功标记
static bool resolveSuccessed = false;

//自动解析模板
template<typename FuncPointer>
bool resolveSymbol(QLibrary &lib, const char *symbolName, FuncPointer* pointerRet)
{
    *pointerRet = reinterpret_cast<FuncPointer>(lib.resolve(symbolName));
    return *pointerRet != nullptr;
}

QString libPath(const QString &strlib)
{
    QDir dir;
    QString path  = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    dir.setPath(path);
    QStringList list = dir.entryList(QStringList() << (strlib + "*"), QDir::NoDotAndDotDot | QDir::Files); //filter name with strlib
    if (list.contains(strlib)) {
        return strlib;
    } else {
        list.sort();
    }

    if(list.size() > 0) {
        return list.last();
    } else {
        return QString();
    }
}

//解析错误判断
#define checkIfFalse(func) if((func) == false)break

bool resolveSymbols()
{
    QLibrary gstLib(libPath("libgstreamer-1.0.so"));

    do {
        checkIfFalse(resolveSymbol(gstLib, "gst_init", &gst_init));
        checkIfFalse(resolveSymbol(gstLib, "gst_caps_get_size", &gst_caps_get_size));
        checkIfFalse(resolveSymbol(gstLib, "gst_caps_get_structure", &gst_caps_get_structure));
        checkIfFalse(resolveSymbol(gstLib, "gst_structure_has_name", &gst_structure_has_name));
        checkIfFalse(resolveSymbol(gstLib, "gst_element_factory_make", &gst_element_factory_make));
        checkIfFalse(resolveSymbol(gstLib, "gst_object_unref", &gst_object_unref));
        checkIfFalse(resolveSymbol(gstLib, "gst_pipeline_new", &gst_pipeline_new));
        checkIfFalse(resolveSymbol(gstLib, "gst_bin_add_many", &gst_bin_add_many));
        checkIfFalse(resolveSymbol(gstLib, "gst_element_link_many", &gst_element_link_many));
        checkIfFalse(resolveSymbol(gstLib, "gst_bin_get_type", &gst_bin_get_type));
        checkIfFalse(resolveSymbol(gstLib, "gst_sample_get_buffer", &gst_sample_get_buffer));
        checkIfFalse(resolveSymbol(gstLib, "gst_sample_get_caps", &gst_sample_get_caps));
        checkIfFalse(resolveSymbol(gstLib, "gst_caps_new_empty", &gst_caps_new_empty));
        checkIfFalse(resolveSymbol(gstLib, "gst_structure_copy", &gst_structure_copy));
        checkIfFalse(resolveSymbol(gstLib, "gst_structure_remove_field", &gst_structure_remove_field));
        checkIfFalse(resolveSymbol(gstLib, "gst_caps_append_structure", &gst_caps_append_structure));
        checkIfFalse(resolveSymbol(gstLib, "gst_element_set_state", &gst_element_set_state));
        checkIfFalse(resolveSymbol(gstLib, "gst_mini_object_unref", &gst_mini_object_unref));
        checkIfFalse(resolveSymbol(gstLib, "gst_element_get_bus", &gst_element_get_bus));
        checkIfFalse(resolveSymbol(gstLib, "gst_bus_timed_pop_filtered", &gst_bus_timed_pop_filtered));
        checkIfFalse(resolveSymbol(gstLib, "gst_message_parse_error", &gst_message_parse_error));
        checkIfFalse(resolveSymbol(gstLib, "gst_bin_get_by_name", &gst_bin_get_by_name));
        checkIfFalse(resolveSymbol(gstLib, "gst_structure_get_int", &gst_structure_get_int));
        checkIfFalse(resolveSymbol(gstLib, "gst_buffer_get_memory", &gst_buffer_get_memory));
        checkIfFalse(resolveSymbol(gstLib, "gst_memory_unmap", &gst_memory_unmap));
        checkIfFalse(resolveSymbol(gstLib, "gst_element_seek", &gst_element_seek));
        checkIfFalse(resolveSymbol(gstLib, "gst_element_get_state", &gst_element_get_state));
        checkIfFalse(resolveSymbol(gstLib, "gst_element_query_duration", &gst_element_query_duration));
        checkIfFalse(resolveSymbol(gstLib, "gst_memory_map", &gst_memory_map));
        checkIfFalse(resolveSymbol(gstLib, "gst_tag_list_get_string_index", &gst_tag_list_get_string_index));
        checkIfFalse(resolveSymbol(gstLib, "gst_caps_new_simple", &gst_caps_new_simple));
        checkIfFalse(resolveSymbol(gstLib, "_gst_fraction_type", &_gst_fraction_type));
        resolveSuccessed = true;
    }while(0);

    return resolveSuccessed;
}

struct ThumbApp {
	GstElement *play;
    gint64 duration;
};

enum FrameType {
    RAW,
    GL
};

static bool capsAreRaw(const GstCaps * caps)
{
    unsigned int len = gst_caps_get_size(caps);
    for (unsigned int i = 0; i < len; i++) {
        GstStructure *st = gst_caps_get_structure(caps, i);
        if (gst_structure_has_name(st, "video/x-raw"))
            return true;
    }
    return false;
}

static bool createElement(const gchar *factoryName, GstElement **element)
{
    *element = gst_element_factory_make(factoryName, nullptr);
    return *element != nullptr;
}

static GstElement *buildPipeline(FrameType captureType, GstElement **srcElement, GstElement **sinkElement,
                                 const GstCaps *fromCaps, const GstCaps *to_caps, GError **err)
{
    if (!capsAreRaw(to_caps)) {
        return nullptr;
    }

    GstElement *csp = nullptr;
    GstElement *vscale = nullptr;
    GstElement *src = nullptr;
    GstElement *sink = nullptr;
    GstElement *dl = nullptr;
    bool noElements = false;
    do {
        if (!createElement("appsrc", &src) ||
                !createElement("appsink", &sink)) {
            noElements = true;
            break;
        }

        if (captureType == RAW) {
            if (!createElement("videoconvert", &csp) ||
                    !createElement("videoscale", &vscale)) {
                noElements = true;
                break;
            }
        } else {
            if (!createElement("glcolorconvert", &csp) ||
                    !createElement("glcolorscale", &vscale) ||
                    !createElement("gldownload", &dl)) {
                noElements = true;
                break;
            }
        }
    }while(0);

    if(noElements) {
        if (src) {
            gst_object_unref(src);
        }

        if (csp) {
            gst_object_unref(csp);
        }

        if (vscale) {
            gst_object_unref(vscale);
        }

        if (dl) {
            gst_object_unref(dl);
        }

        if (sink) {
            gst_object_unref(sink);
        }
        return nullptr;
    }

    GstElement *pipeline = gst_pipeline_new ("videoconvert-pipeline");
    if (pipeline == nullptr) {
        gst_object_unref(src);
        gst_object_unref(csp);
        gst_object_unref(vscale);
        g_clear_pointer(&dl, gst_object_unref);
        gst_object_unref(sink);
        return nullptr;
    }

    if (g_object_class_find_property(G_OBJECT_GET_CLASS (vscale), "add-borders")) {
        g_object_set(vscale, "add-borders", TRUE, nullptr);
    }

    gst_bin_add_many(GST_BIN (pipeline), src, csp, vscale, sink, dl, nullptr);
    g_object_set(src, "caps", fromCaps, nullptr);
    g_object_set(sink, "caps", to_caps, nullptr);

    bool ret;
    if (dl) {
        ret = gst_element_link_many(src, csp, vscale, dl, sink, nullptr);
    } else {
        ret = gst_element_link_many(src, csp, vscale, sink, nullptr);
    }

    if (!ret) {
        gst_object_unref(pipeline);
        return nullptr;
    }

    g_object_set(src, "emit-signals", TRUE, nullptr);
    g_object_set(sink, "emit-signals", TRUE, nullptr);
    *srcElement = src;
    *sinkElement = sink;
    return pipeline;
}

static GstSample *getVideoSample(FrameType captureType, GstSample *sample,
                                  const GstCaps *toCaps, GstClockTime timeout, GError **error)
{
    if(sample == nullptr || toCaps == nullptr) {
        return nullptr;
    }

    GstBuffer *buf = gst_sample_get_buffer(sample);
    if(buf == nullptr) {
        return nullptr;
    }

    GstCaps *fromCaps = gst_sample_get_caps(sample);
    if(fromCaps == nullptr) {
        return nullptr;
    }

    GstCaps *toCapsCopy = gst_caps_new_empty();
    size_t n = gst_caps_get_size(toCaps);
    for (unsigned int i = 0; i < n; i++) {
        GstStructure *s = gst_caps_get_structure(toCaps, i);

        s = gst_structure_copy (s);
        gst_structure_remove_field (s, "framerate");
        gst_caps_append_structure (toCapsCopy, s);
    }

    GError *err = nullptr;
    GstElement *sink;
    GstElement *src;
    GstElement *pipeline = buildPipeline(captureType, &src, &sink, fromCaps, toCapsCopy, &err);
    if (!pipeline || gst_element_set_state(pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        gst_mini_object_unref(GST_MINI_OBJECT_CAST(toCapsCopy));
        if (error) {
            *error = err;
        } else {
            g_error_free (err);
        }
        return nullptr;
    }

    GstFlowReturn ret;
    g_signal_emit_by_name (src, "push-buffer", buf, &ret);

    GstBus *bus = gst_element_get_bus (pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered (bus, timeout, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_ASYNC_DONE));

    GstSample *result = nullptr;
    if (msg) {
        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ASYNC_DONE: {
            g_signal_emit_by_name (sink, "pull-preroll", &result);
            break;
        }
        case GST_MESSAGE_ERROR: {
            gchar *dbg = nullptr;

            gst_message_parse_error (msg, &err, &dbg);
            if (err) {
                if (error) {
                    *error = err;
                } else {
                    g_error_free (err);
                }
            }
            g_free (dbg);
            break;
        }
        default:
            g_return_val_if_reached(nullptr);
        }
        gst_mini_object_unref(GST_MINI_OBJECT_CAST(msg));
    }

    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (bus);
    gst_object_unref (pipeline);
    gst_mini_object_unref(GST_MINI_OBJECT_CAST(toCapsCopy));

    return result;
}

static QImage getImageFromPlayer(GstElement *play, GError **error)
{
    QImage result;

    FrameType captureType = gst_bin_get_by_name(GST_BIN (play), "glcolorbalance0") ? GL : RAW;

    GstCaps *toCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING,
                                           captureType == RAW ? "RGB" : "RGBA",
                                           "pixel-aspect-ratio", *_gst_fraction_type, 1, 1, nullptr);

    GstSample *lastSample = nullptr;
    g_object_get(G_OBJECT (play), "sample", &lastSample, nullptr);
    if (!lastSample) {
        return result;
    }
    GstSample *sample = getVideoSample(captureType, lastSample, toCaps, 25 * GST_SECOND, error);
    gst_mini_object_unref(GST_MINI_OBJECT_CAST(lastSample));
    gst_mini_object_unref(GST_MINI_OBJECT_CAST(toCaps));

    if (!sample) {
        return result;
    }

    GstCaps *sampleCaps = gst_sample_get_caps(sample);
    if (!sampleCaps) {
        return result;
    }

    GstStructure *s = gst_caps_get_structure(sampleCaps, 0);
    int outwidth = 0;
    int outheight = 0;
    gst_structure_get_int(s, "width", &outwidth);
    gst_structure_get_int(s, "height", &outheight);
    if (outwidth > 0 && outheight > 0) {
        GstMemory *memory = gst_buffer_get_memory(gst_sample_get_buffer (sample), 0);
        GstMapInfo info;
        gst_memory_map(memory, &info, GST_MAP_READ);

        result = QImage(info.data, outwidth, outheight, QImage::Format_RGB888);
        gst_memory_unmap(memory, &info);
        gst_mini_object_unref(GST_MINI_OBJECT_CAST(memory));
    }

    if (result.isNull()) {
        gst_mini_object_unref(GST_MINI_OBJECT_CAST(sample));
    }

    int rotation = 0;
    if (g_object_get_data(G_OBJECT (play), "orientation-checked") == nullptr) {
        GstTagList *tags = nullptr;

        g_signal_emit_by_name(G_OBJECT (play), "get-video-tags", 0, &tags);
        if (tags) {
            char *orientation_str;
            bool ret = gst_tag_list_get_string_index(tags, "image-orientation", 0, &orientation_str);
            if (!ret || !orientation_str) {
                rotation = 0;
            } else if (g_str_equal(orientation_str, "rotate-90")) {
                rotation = 90;
            } else if (g_str_equal(orientation_str, "rotate-180")) {
                rotation = 180;
            } else if (g_str_equal(orientation_str, "rotate-270")) {
                rotation = 270;
            }
            gst_mini_object_unref(GST_MINI_OBJECT_CAST(tags));
        }

        g_object_set_data(G_OBJECT (play), "orientation-checked", GINT_TO_POINTER(1));
        g_object_set_data(G_OBJECT (play), "orientation", GINT_TO_POINTER(rotation));
    }

    QMatrix rotateMatrix;
    rotateMatrix.rotate(rotation);
    result = result.transformed(rotateMatrix, Qt::SmoothTransformation);

    return result;
}

static bool startApp(ThumbApp *app)
{
	gst_element_set_state (app->play, GST_STATE_PAUSED);
    GstBus *bus = gst_element_get_bus (app->play);
    GstMessageType events = static_cast<GstMessageType>(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_ERROR);
    bool needStop = false;
    bool asyncHaveReceived = false;
    while(!needStop) {
        GstMessage *message = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, events); //主要耗时点
        GstElement *src = (GstElement*)GST_MESSAGE_SRC (message);
		switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ASYNC_DONE:
			if (src == app->play) {
                asyncHaveReceived = true;
                needStop = true;
			}
			break;
        case GST_MESSAGE_ERROR:
            needStop = true;
			break;
		default:
            break;
        }
        gst_mini_object_unref(GST_MINI_OBJECT_CAST(message));
	}
    gst_object_unref (bus);
    return asyncHaveReceived;
}

static void buildSink(ThumbApp *app)
{
    //设置视频播放器
    GstElement *play = gst_element_factory_make ("playbin", "play");

    //设置视频数据管道
    GstElement *videoSink = gst_element_factory_make ("fakesink", "video-fake-sink");
    g_object_set(videoSink, "sync", true, nullptr);

    //关闭视频前台播放
    g_object_set(play, "video-sink", videoSink, "flags", 1, nullptr);

    app->play = play;
}

static bool testImage(const QImage &image)
{
    //算法原理：通过统计整个图片像素点的方差来确认图片的鲜艳程度，方差越大图片整体色彩越鲜艳，也就越适合做缩略图

    //1.提取像素点
    const uchar* buffer = image.bits();
    int pixelCount = image.bytesPerLine() * image.height();
    float meanValue = 0.0f;
    float variance = 0.0f;

    //2.计算均值
    for (int i = 0; i < pixelCount; i++) {
        meanValue += static_cast<float>(buffer[i]);
    }
    meanValue /= static_cast<float>(pixelCount);

    //3.计算方差
    for (int i = 0; i < pixelCount; i++) {
        float tmp = static_cast<float>(buffer[i]) - meanValue;
        variance += tmp * tmp;
    }
    variance /= static_cast<float>(pixelCount - 1);

    //4.当方差大于设定的阈值时，即表示该图片适合做缩略图
    return variance > 256.0f;
}

static QImage getImage(ThumbApp *app)
{
    //图片截取流程

    //1.如果无法读取视频的长度，则直接取第一帧作为结果
    if (app->duration == -1) {
        return getImageFromPlayer(app->play, nullptr);
	}

    //2.设置备选点位
    const double pos[] = { 0.1, 1.0 / 3.0, 0.5, 2.0 / 3.0, 0.9 };
    QImage image;

    //3.点位筛选循环
    for (int i = 0; i < G_N_ELEMENTS(pos); i++) {
        //3.1跳转至目标点位
        gst_element_seek (app->play, 1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                          GST_SEEK_TYPE_SET, pos[i] * app->duration * GST_MSECOND, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
        gst_element_get_state (app->play, nullptr, nullptr, GST_CLOCK_TIME_NONE);

        //3.2执行截图
        image = getImageFromPlayer(app->play, nullptr);

        //3.3如果截图成功且截图满足要求
        if (!image.isNull() && testImage(image)) {
			break;
        }
	}

    //4.返回图片
    return image;
}

QImage runGstreamerVideoThumbnailer(const QUrl &videoUrl)
{
    QImage result;

    if(!resolveSuccessed) {
        return result;
    }

    //读取输入视频路径和输出图像路径
    //视频路径需要用URL格式，图像路径需要用一般绝对路径格式
    ThumbApp app;

    //建立管道
    buildSink(&app);

    //对播放器设置输入文件名
    g_object_set(app.play, "uri", videoUrl.url().toStdString().c_str(), nullptr);

    //打开视频文件（主要耗时点）
    //此处会自动分析是否是有效的视频文件
    if (startApp(&app) == false) {
        return result;
    }

    //读取并设置视频时长
    gint64 len = -1;
    if (gst_element_query_duration (app.play, GST_FORMAT_TIME, &len) && len != -1) {
        app.duration = len / GST_MSECOND;
    } else {
        app.duration = -1;
    }

    //抓取图片
    result = getImage(&app);

    //释放播放器资源
    gst_element_set_state(app.play, GST_STATE_NULL);
    g_clear_object(&app.play);

    return result;
}

void initGstreamerVideoThumbnailer()
{
    if(resolveSymbols()) {
        int argc = 0;
        char **argv = {};
        gst_init(&argc, &argv);
    }
}
