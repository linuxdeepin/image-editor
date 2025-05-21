// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "snifferimageformat.h"

#include <QDebug>
#include <QFile>

// For more information about image file extension, see:
// https://en.wikipedia.org/wiki/Image_file_formats
QString DetectImageFormat(const QString &filepath)
{
    qDebug() << "Starting image format detection for:" << filepath;
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for format detection:" << filepath;
        return "";
    }

    const QByteArray data = file.read(1024);
    qDebug() << "Read" << data.size() << "bytes for format detection";

    // Check bmp file.
    if (data.startsWith("BM")) {
        qDebug() << "Detected BMP format";
        return "bmp";
    }

    // Check dds file.
    if (data.startsWith("DDS")) {
        qDebug() << "Detected DDS format";
        return "dds";
    }

    // Check gif file.
    if (data.startsWith("GIF8")) {
        qDebug() << "Detected GIF format";
        return "gif";
    }

    // Check Max OS icons file.
    if (data.startsWith("icns")) {
        qDebug() << "Detected ICNS format";
        return "icns";
    }

    // Check jpeg file.
    if (data.startsWith("\xff\xd8")) {
        qDebug() << "Detected JPEG format";
        return "jpg";
    }

    // Check mng file.
    if (data.startsWith("\x8a\x4d\x4e\x47\x0d\x0a\x1a\x0a")) {
        qDebug() << "Detected MNG format";
        return "mng";
    }

    // Check net pbm file (BitMap).
    if (data.startsWith("P1") || data.startsWith("P4")) {
        qDebug() << "Detected PBM format";
        return "pbm";
    }

    // Check pgm file (GrayMap).
    if (data.startsWith("P2") || data.startsWith("P5")) {
        qDebug() << "Detected PGM format";
        return "pgm";
    }

    // Check ppm file (PixMap).
    if (data.startsWith("P3") || data.startsWith("P6")) {
        qDebug() << "Detected PPM format";
        return "ppm";
    }

    // Check png file.
    if (data.startsWith("\x89PNG\x0d\x0a\x1a\x0a")) {
        qDebug() << "Detected PNG format";
        return "png";
    }

    // Check svg file.
    if (data.indexOf("<svg") > -1) {
        qDebug() << "Detected SVG format";
        return "svg";
    }

    // TODO(xushaohua): tga file is not supported yet.
    qDebug() << "TGA format detection not implemented yet";

    // Check tiff file.
    if (data.startsWith("MM\x00\x2a") || data.startsWith("II\x2a\x00")) {
        // big-endian, little-endian.
        qDebug() << "Detected TIFF format";
        return "tiff";
    }

    // TODO(xushaohua): Support wbmp file.
    qDebug() << "WBMP format detection not implemented yet";

    // Check webp file.
    if (data.startsWith("RIFFr\x00\x00\x00WEBPVP")) {
        qDebug() << "Detected WebP format";
        return "webp";
    }

    // Check xbm file.
    if (data.indexOf("#define max_width ") > -1 &&
            data.indexOf("#define max_height ") > -1) {
        qDebug() << "Detected XBM format";
        return "xbm";
    }

    // Check xpm file.
    if (data.startsWith("/* XPM */")) {
        qDebug() << "Detected XPM format";
        return "xpm";
    }

    qDebug() << "No supported image format detected";
    return "";
}
