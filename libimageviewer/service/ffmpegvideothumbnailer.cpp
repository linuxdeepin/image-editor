// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ffmpegvideothumbnailer.h"

#include <QApplication>

#include <QString>
#include <QLibrary>
#include <QDir>
#include <QLibraryInfo>
#include <QDebug>

#include <libffmpegthumbnailer/videothumbnailerc.h>

typedef video_thumbnailer *(*mvideo_thumbnailer_create)();
typedef void (*mvideo_thumbnailer_destroy)(video_thumbnailer *thumbnailer);
typedef image_data *(*mvideo_thumbnailer_create_image_data)(void);
typedef void (*mvideo_thumbnailer_destroy_image_data)(image_data *data);
typedef int (*mvideo_thumbnailer_generate_thumbnail_to_buffer)(video_thumbnailer *thumbnailer, const char *movie_filename, image_data *generated_image_data);

static mvideo_thumbnailer_create m_creat_video_thumbnailer = nullptr;
static mvideo_thumbnailer_destroy m_mvideo_thumbnailer_destroy = nullptr;
static mvideo_thumbnailer_create_image_data m_mvideo_thumbnailer_create_image_data = nullptr;
static mvideo_thumbnailer_destroy_image_data m_mvideo_thumbnailer_destroy_image_data = nullptr;
static mvideo_thumbnailer_generate_thumbnail_to_buffer m_mvideo_thumbnailer_generate_thumbnail_to_buffer = nullptr;

static video_thumbnailer *m_video_thumbnailer = nullptr;

//解析成功标记
static bool resolveSuccessed = false;

/**
   @brief 根据传入的库名称 \a strlib 查找 LibrariesPath 目录下的动态库，
        并返回查找到的库名称，这个函数是由于 QLibrary 不会对带后缀的库(*.so.4.11)
        查找而提供的。
    例如:
        当环境中仅提供 libffmpegthumbnailer.so.4 而不存在 libffmpegthumbnailer.so 时,
        QLibrary 抛出错误，此函数会返回 libffmpegthumbnailer.so.4 的名称。
 */
static QString libPath(const QString &strlib)
{
    QDir dir;
    QString path = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    dir.setPath(path);
    QStringList list = dir.entryList(QStringList() << (strlib + "*"), QDir::NoDotAndDotDot | QDir::Files); //filter name with strlib
    if (list.contains(strlib)) {
        return strlib;
    } else {
        list.sort();
    }

    if (list.size() > 0) {
        return list.last();
    } else {
        return QString();
    }
}

/**
   @return 初始化 ffmpeg 视频缩略图库并返回是否成功加载。
        将尝试查找 ffmpegthumbnailer 并解析暴露函数，

   @warning libffmpegthumbnailer.so.4 只对应软件包 libffmpegthumbnailer4v5 ,
        若后续软件包更新，需要考虑能否正常查找对应动态库。
 */
bool initFFmpegVideoThumbnailer()
{
    if (resolveSuccessed) {
        return true;
    }

    // 没有显式调用 unload() ，动态库会保存在内存中，直到程序结束。resolve() 在内部会自动调用 load() 加载。
    QLibrary library("libffmpegthumbnailer.so.4");
    if (!library.load()) {
        qWarning() << QString("Find libffmpegthumbnailer.so failed by default, error: %1").arg(library.errorString());

        // 默认查找失败，尝试手动查找目录
        QString findLib = libPath("libffmpegthumbnailer.so");
        if (findLib.isEmpty()) {
            qWarning() << QString("Can not find libffmpegthumbnailer.so, LibrariesPath: %1")
                          .arg(QLibraryInfo::location(QLibraryInfo::LibrariesPath));
            return false;
        } else {
            qInfo() << QString("Current find ffmpegthumbnailer lib is %1, LibrariesPath: %2").arg(findLib)
                          .arg(QLibraryInfo::location(QLibraryInfo::LibrariesPath));
        }

        library.setFileName(findLib);
        if (!library.load()) {
            qWarning() << QString("Find libffmpegthumbnailer.so failed by find path, error: %1").arg(library.errorString());
            return false;
        }
    }

    m_creat_video_thumbnailer = reinterpret_cast<mvideo_thumbnailer_create>(library.resolve("video_thumbnailer_create"));
    m_mvideo_thumbnailer_destroy = reinterpret_cast<mvideo_thumbnailer_destroy>(library.resolve("video_thumbnailer_destroy"));
    m_mvideo_thumbnailer_create_image_data = reinterpret_cast<mvideo_thumbnailer_create_image_data>(library.resolve("video_thumbnailer_create_image_data"));
    m_mvideo_thumbnailer_destroy_image_data = reinterpret_cast<mvideo_thumbnailer_destroy_image_data>(library.resolve("video_thumbnailer_destroy_image_data"));
    m_mvideo_thumbnailer_generate_thumbnail_to_buffer = reinterpret_cast<mvideo_thumbnailer_generate_thumbnail_to_buffer>(library.resolve("video_thumbnailer_generate_thumbnail_to_buffer"));

    if (nullptr == m_creat_video_thumbnailer) {
        qWarning() << QString("Resolve libffmpegthumbnailer.so data failed, %1").arg(library.errorString());
        return false;
    }
    m_video_thumbnailer = m_creat_video_thumbnailer();

    if (m_mvideo_thumbnailer_destroy == nullptr
            || m_mvideo_thumbnailer_create_image_data == nullptr
            || m_mvideo_thumbnailer_destroy_image_data == nullptr
            || m_mvideo_thumbnailer_generate_thumbnail_to_buffer == nullptr
            || m_video_thumbnailer == nullptr) {
        qWarning() << QString("Resolve libffmpegthumbnailer.so create video thumbnailer failed, %1")
                      .arg(library.errorString());
        return false;
    }

    resolveSuccessed = true;
    return true;
}

/**
   @return 根据传入文件路径 \a url 创建视频缩略图并返回，若未成功解析 libffmpegthumbnailer.so
        动态库，将返回空图片
 */
QImage runFFmpegVideoThumbnailer(const QUrl &url)
{
    if (!resolveSuccessed) {
        return QImage();
    }

    m_video_thumbnailer->thumbnail_size = static_cast<int>(400 * qApp->devicePixelRatio());
    image_data *image_data = m_mvideo_thumbnailer_create_image_data();
    QString file = QFileInfo(url.toLocalFile()).absoluteFilePath();
    m_mvideo_thumbnailer_generate_thumbnail_to_buffer(m_video_thumbnailer, file.toUtf8().data(), image_data);
    QImage img = QImage::fromData(image_data->image_data_ptr, static_cast<int>(image_data->image_data_size), "png");
    m_mvideo_thumbnailer_destroy_image_data(image_data);
    image_data = nullptr;
    return img;
}

