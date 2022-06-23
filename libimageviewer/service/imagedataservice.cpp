/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
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
#include "imagedataservice.h"

#include <QMetaType>
#include <QDirIterator>
#include <QStandardPaths>
#include <QImageReader>
#include <QDebug>

#include <mutex>

#include "unionimage/pluginbaseutils.h"
#include "unionimage/unionimage.h"
#include "unionimage/baseutils.h"
#include "unionimage/imageutils.h"
#include "commonservice.h"

LibImageDataService *LibImageDataService::s_ImageDataService = nullptr;
static std::once_flag dataServiceFlag;

LibImageDataService *LibImageDataService::instance(QObject *parent)
{
    Q_UNUSED(parent);

    std::call_once(dataServiceFlag, []() {
        s_ImageDataService = new LibImageDataService();
    });

    return s_ImageDataService;
}

LibImageDataService::~LibImageDataService()
{
    stopReadThumbnail();
}

bool LibImageDataService::add(const QStringList &paths)
{
    QMutexLocker locker(&m_imgDataMutex);
//    m_requestQueue.clear();
    for (int i = 0; i < paths.size(); i++) {
        if (!m_AllImageMap.contains(paths.at(i))) {
            m_requestQueue.append(paths.at(i));
        }
    }
    return true;
}

bool LibImageDataService::add(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    if (!path.isEmpty()) {
        if (!m_AllImageMap.contains(path)) {
            m_requestQueue.append(path);
        }
    }
    return true;
}

QString LibImageDataService::pop()
{
    QMutexLocker locker(&m_imgDataMutex);
    if (m_requestQueue.empty())
        return QString();
    QString res = m_requestQueue.first();
    m_requestQueue.pop_front();
    return res;
}

bool LibImageDataService::isRequestQueueEmpty()
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_requestQueue.isEmpty();
}

int LibImageDataService::getCount()
{
    return m_AllImageMap.count();
}

bool LibImageDataService::readThumbnailByPaths(QString thumbnailPath, QStringList files, bool remake)
{
    qDebug() << "------------files.size = " << files.size();

    LibImageDataService::instance()->add(files);

    int needCoreCounts = static_cast<int>(std::thread::hardware_concurrency());
    needCoreCounts = needCoreCounts / 2;
    if (files.size() < needCoreCounts) {
        needCoreCounts = files.size();
    }
    if (needCoreCounts < 1)
        needCoreCounts = 1;
    for (int i = 0; i < needCoreCounts; i++) {
        LibReadThumbnailThread *thread = new LibReadThumbnailThread;
        thread->m_thumbnailPath = thumbnailPath;
        thread->m_remake = remake;
        thread->start();
        readThreadGroup.push_back(thread);
    }
    return true;
}

#include "imageengine.h"
void LibImageDataService::addImage(const QString &path, const QImage &image)
{
    QMutexLocker locker(&m_imgDataMutex);
    m_AllImageMap[path] = image;
    qDebug() << "------------m_requestQueue.size = " << m_requestQueue.size();
    qDebug() << "------------m_AllImageMap.size = " << m_AllImageMap.size();

//    emit ImageEngine::instance()->sigOneImgReady(path, info);

//    if (!m_AllImageMap.contains(path)) {
//        m_AllImageMap[path] = image;
//        while (m_AllImageMap.size() > 1000) {
//            //保证缓存占用，始终只占用1000张缩略图缓存
//            QString res = m_imageKey.first();
//            m_imageKey.pop_front();
//            m_AllImageMap.remove(res);
//        }
//    }
}

void LibImageDataService::addMovieDurationStr(const QString &path, const QString &durationStr)
{
    QMutexLocker locker(&m_imgDataMutex);
    m_movieDurationStrMap[path] = durationStr;
}

QString LibImageDataService::getMovieDurationStrByPath(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_movieDurationStrMap.contains(path) ? m_movieDurationStrMap[path] : QString() ;
}

void LibImageDataService::setAllDataKeys(const QStringList &paths, bool single)
{
    Q_UNUSED(paths);
    Q_UNUSED(single);
}

void LibImageDataService::setVisualIndex(int row)
{
    QMutexLocker locker(&m_imgDataMutex);
    m_visualIndex = row;
}

int LibImageDataService::getVisualIndex()
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_visualIndex;
}

QImage LibImageDataService::getThumnailImageByPath(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_AllImageMap.contains(path) ? m_AllImageMap[path] : QImage();
}

bool LibImageDataService::imageIsLoaded(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_AllImageMap.contains(path);
}

LibImageDataService::LibImageDataService(QObject *parent)
{
    Q_UNUSED(parent);
}

void LibImageDataService::stopReadThumbnail()
{
    if (!readThreadGroup.empty()) {
        for (auto &thread : readThreadGroup) {
            thread->setQuit(true);
        }

        for (auto &thread : readThreadGroup) {
            while (thread->isRunning());
            thread->deleteLater();
        }

        readThreadGroup.clear();
    }
}

//缩略图读取线程
LibReadThumbnailThread::LibReadThumbnailThread(QObject *parent)
    : QThread(parent)
{
    m_quit = false;
}

void LibReadThumbnailThread::readThumbnail(QString path)
{
    if (!QFileInfo(path).exists()) {
        return;
    }
    //新增,增加缓存
    imageViewerSpace::ItemInfo itemInfo;

    itemInfo.path = path;

    using namespace LibUnionImage_NameSpace;
    QImage tImg;
    QString errMsg;

    if (!LibUnionImage_NameSpace::loadStaticImageFromFile(path, tImg, errMsg)) {
        qDebug() << errMsg;
        //损坏图片也需要缓存更新
        itemInfo.imageType = imageViewerSpace::ImageTypeDamaged;
        LibCommonService::instance()->slotSetImgInfoByPath(path, itemInfo);
        return;
    }
    //读取图片,给长宽重新赋值
    itemInfo.imgOriginalWidth = tImg.width();
    itemInfo.imgOriginalHeight = tImg.height();

    if (0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
        bool cache_exist = false;
        if (tImg.height() != 200 && tImg.width() != 200) {
            if (tImg.height() >= tImg.width()) {
                cache_exist = true;
                tImg = tImg.scaledToWidth(800,  Qt::FastTransformation);
                tImg = tImg.scaledToWidth(200,  Qt::SmoothTransformation);
            } else if (tImg.height() <= tImg.width()) {
                cache_exist = true;
                tImg = tImg.scaledToWidth(800,  Qt::FastTransformation);
                tImg = tImg.scaledToWidth(200,  Qt::SmoothTransformation);
            }
        }
        if (!cache_exist) {
            if (static_cast<float>(tImg.height()) / static_cast<float>(tImg.width()) > 3) {
                tImg = tImg.scaledToWidth(800,  Qt::FastTransformation);
                tImg = tImg.scaledToWidth(200,  Qt::SmoothTransformation);
            } else {
                tImg = tImg.scaledToWidth(800,  Qt::FastTransformation);
                tImg = tImg.scaledToWidth(200,  Qt::SmoothTransformation);
            }
        }
    }
    else {
        tImg = tImg.scaled(200,200);
    }

    itemInfo.image = tImg;

    if (itemInfo.image.isNull()) {
        itemInfo.imageType = imageViewerSpace::ImageTypeDamaged;
    } else {
        //获取图片类型
        itemInfo.imageType = getImageType(path);
    }
    LibCommonService::instance()->slotSetImgInfoByPath(path, itemInfo);
}

void LibReadThumbnailThread::setQuit(bool quit)
{
    m_quit = quit;
}

imageViewerSpace::ImageType LibReadThumbnailThread::getImageType(const QString &imagepath)
{
    return LibUnionImage_NameSpace::getImageType(imagepath);
}

imageViewerSpace::PathType LibReadThumbnailThread::getPathType(const QString &imagepath)
{
    return LibUnionImage_NameSpace::getPathType(imagepath);
}

void LibReadThumbnailThread::run()
{
    while (!LibImageDataService::instance()->isRequestQueueEmpty()) {
        if (m_quit) {
            break;
        }
        QString res = LibImageDataService::instance()->pop();
        if (!res.isEmpty()) {
            readThumbnail(res);
        }
    }
    emit LibImageDataService::instance()->sigeUpdateListview();
}
