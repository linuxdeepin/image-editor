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

#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gst/gst.h>

#include <string.h>
#include <math.h>
#include <stdlib.h>

#include <QUrl>
#include <QImage>

//参考自totem项目

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
    *element = gst_element_factory_make (factoryName, nullptr);
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
        if (err) {
            *err = g_error_new(GST_CORE_ERROR, GST_CORE_ERROR_FAILED, "Could not convert video frame: no pipeline (unknown error)");
        }
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
        if (err) {
            *err = g_error_new(GST_CORE_ERROR, GST_CORE_ERROR_NEGOTIATION, "Could not convert video frame: failed to link elements");
        }
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
        gst_caps_unref (toCapsCopy);
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
        gst_message_unref (msg);
    } else {
        if (error) {
            *error = g_error_new (GST_CORE_ERROR, GST_CORE_ERROR_FAILED,
                                  "Could not convert video frame: timeout during conversion");
        }
    }

    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (bus);
    gst_object_unref (pipeline);
    gst_caps_unref (toCapsCopy);

    return result;
}

static QImage getImageFromPlayer(GstElement *play, GError **error)
{
    QImage result;

    FrameType captureType = gst_bin_get_by_name(GST_BIN (play), "glcolorbalance0") ? GL : RAW;

    GstCaps *toCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING,
                                           captureType == RAW ? "RGB" : "RGBA",
                                           "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, nullptr);

    GstSample *lastSample = nullptr;
    g_object_get(G_OBJECT (play), "sample", &lastSample, nullptr);
    if (!lastSample) {
        return result;
    }
    GstSample *sample = getVideoSample(captureType, lastSample, toCaps, 25 * GST_SECOND, error);
    gst_sample_unref(lastSample);
    gst_caps_unref(toCaps);

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
        gst_memory_unref(memory);
    }

    if (result.isNull()) {
        gst_sample_unref(sample);
    }

    int rotation = 0;
    if (g_object_get_data(G_OBJECT (play), "orientation-checked") == nullptr) {
        GstTagList *tags = nullptr;

        g_signal_emit_by_name(G_OBJECT (play), "get-video-tags", 0, &tags);
        if (tags) {
            char *orientation_str;
            bool ret = gst_tag_list_get_string_index(tags, GST_TAG_IMAGE_ORIENTATION, 0, &orientation_str);
            if (!ret || !orientation_str) {
                rotation = 0;
            } else if (g_str_equal(orientation_str, "rotate-90")) {
                rotation = 90;
            } else if (g_str_equal(orientation_str, "rotate-180")) {
                rotation = 180;
            } else if (g_str_equal(orientation_str, "rotate-270")) {
                rotation = 270;
            }
            gst_tag_list_unref (tags);
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
        gst_message_unref (message);
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
    int argc = 0;
    char **argv = {};
    gst_init(&argc, &argv);
}
