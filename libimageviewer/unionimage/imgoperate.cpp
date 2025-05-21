// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imgoperate.h"

#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QStandardPaths>
#include <QDirIterator>
#include <QThread>
#include <QImage>
#include <QImageReader>
#include <QMimeDatabase>

#include "unionimage.h"
#include "pluginbaseutils.h"
#include "imageengine.h"
#include "imageutils.h"


LibImgOperate::LibImgOperate(QObject *parent)
{
    Q_UNUSED(parent);
    qDebug() << "Initializing LibImgOperate";
}

LibImgOperate::~LibImgOperate()
{
    qDebug() << "Destroying LibImgOperate";
}

void LibImgOperate::slotMakeImgThumbnail(QString thumbnailSavePath, QStringList paths, int makeCount, bool remake)
{
    qDebug() << "Starting thumbnail generation for" << paths.size() << "images, makeCount:" << makeCount << "remake:" << remake;
    QString path;
    imageViewerSpace::ItemInfo itemInfo;
    QImage tImg;
    QImage tImg_1;
    for (int i = 0; i < paths.size(); i++) {
        //达到制作数量则停止
        if (i == makeCount) {
            qDebug() << "Reached makeCount limit, stopping thumbnail generation";
            break;
        }
        path = paths.at(i);
        qDebug() << "Processing image:" << path;
        itemInfo.path = path;

        //获取路径类型
        itemInfo.pathType = getPathType(path);
        qDebug() << "Path type:" << itemInfo.pathType;

        //获取原图分辨率
        QImageReader imagreader(path);
        itemInfo.imgOriginalWidth = imagreader.size().width();
        itemInfo.imgOriginalHeight = imagreader.size().height();
        qDebug() << "Original image size:" << itemInfo.imgOriginalWidth << "x" << itemInfo.imgOriginalHeight;

        //缩略图保存路径
        QString savePath = thumbnailSavePath + path;
        //保存为jpg格式
        savePath = thumbnailSavePath.mid(0, savePath.lastIndexOf('.')) + ImageEngine::instance()->makeMD5(path) + ".png";
        QFileInfo file(savePath);
        //缩略图已存在，执行下一个路径
        if (file.exists() && !remake && itemInfo.imgOriginalWidth > 0 && itemInfo.imgOriginalHeight > 0) {
            qDebug() << "Using existing thumbnail:" << savePath;
            tImg = QImage(savePath);
            itemInfo.image = tImg;
            //获取图片类型
            itemInfo.imageType = getImageType(path);
            emit sigOneImgReady(path, itemInfo);
            continue;
        }

        QString errMsg;
        if (!LibUnionImage_NameSpace::loadStaticImageFromFile(path, tImg, errMsg)) {
            qWarning() << "Failed to load image:" << path << "Error:" << errMsg;
            continue;
        }
        itemInfo.imgOriginalWidth = tImg.width();
        itemInfo.imgOriginalHeight = tImg.height();
        qDebug() << "Loaded image size:" << itemInfo.imgOriginalWidth << "x" << itemInfo.imgOriginalHeight;

        if (0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
            bool cache_exist = false;
            if (tImg.height() != 200 && tImg.width() != 200) {
                if (tImg.height() >= tImg.width()) {
                    cache_exist = true;
                    tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
                } else if (tImg.height() <= tImg.width()) {
                    cache_exist = true;
                    tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
                }
            }
            if (!cache_exist) {
                if (static_cast<float>(tImg.height()) / static_cast<float>(tImg.width()) > 3) {
                    tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
                } else {
                    tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
                }
            }
            qDebug() << "Scaled thumbnail size:" << tImg.width() << "x" << tImg.height();
        }

        //创建路径
        QString dirPath = savePath.mid(0, savePath.lastIndexOf('/'));
        qDebug() << "Creating directory for thumbnail:" << dirPath;
        pluginUtils::base::mkMutiDir(dirPath);
        
        if (tImg.save(savePath)) {
            qDebug() << "Successfully saved thumbnail:" << savePath;
            itemInfo.image = tImg;
        } else {
            qWarning() << "Failed to save thumbnail:" << savePath;
        }

        if (itemInfo.image.isNull()) {
            qWarning() << "Generated thumbnail is null for:" << path;
            itemInfo.imageType = imageViewerSpace::ImageTypeDamaged;
        } else {
            //获取图片类型
            itemInfo.imageType = getImageType(path);
            qDebug() << "Image type:" << itemInfo.imageType;
        }
        emit sigOneImgReady(path, itemInfo);
    }
    qDebug() << "Finished thumbnail generation";
}

imageViewerSpace::ImageType LibImgOperate::getImageType(const QString &imagepath)
{
    qDebug() << "Getting image type for:" << imagepath;
    return LibUnionImage_NameSpace::getImageType(imagepath);
}

imageViewerSpace::PathType LibImgOperate::getPathType(const QString &imagepath)
{
    qDebug() << "Getting path type for:" << imagepath;
    //判断文件路径来自于哪里
    imageViewerSpace::PathType type = imageViewerSpace::PathType::PathTypeLOCAL;
    if (imagepath.indexOf("smb-share:server=") != -1) {
        type = imageViewerSpace::PathTypeSMB;
        qDebug() << "Path type: SMB share";
    } else if (imagepath.indexOf("mtp:host=") != -1) {
        type = imageViewerSpace::PathTypeMTP;
        qDebug() << "Path type: MTP device";
    } else if (imagepath.indexOf("gphoto2:host=") != -1) {
        type = imageViewerSpace::PathTypePTP;
        qDebug() << "Path type: PTP device";
    } else if (imagepath.indexOf("gphoto2:host=Apple") != -1) {
        type = imageViewerSpace::PathTypeAPPLE;
        qDebug() << "Path type: Apple device";
    } else if (Libutils::image::isVaultFile(imagepath)) {
        type = imageViewerSpace::PathTypeSAFEBOX;
        qDebug() << "Path type: Safe box";
    } else if (imagepath.contains(QDir::homePath() + "/.local/share/Trash")) {
        type = imageViewerSpace::PathTypeRECYCLEBIN;
        qDebug() << "Path type: Recycle bin";
    } else {
        qDebug() << "Path type: Local file";
    }
    return type;
}
