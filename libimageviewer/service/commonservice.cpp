// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commonservice.h"

#include <QImage>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QMutexLocker>
#include <QDebug>

#include "imageengine.h"

LibCommonService *LibCommonService::m_commonService = nullptr;
LibCommonService *LibCommonService::instance()
{
    if (m_commonService == nullptr) {
        qDebug() << "Creating new LibCommonService instance";
        m_commonService = new LibCommonService;
    }

    return m_commonService;
}

void LibCommonService::setImgViewerType(imageViewerSpace::ImgViewerType type)
{
    qDebug() << "Setting image viewer type:" << type;
    m_imgViewerType = type;
}

imageViewerSpace::ImgViewerType LibCommonService::getImgViewerType()
{
    return m_imgViewerType;
}

void LibCommonService::setImgSavePath(QString path)
{
    qDebug() << "Setting image save path:" << path;
    m_imgSavePath = path;
}

QString LibCommonService::getImgSavePath()
{
    return m_imgSavePath;
}

imageViewerSpace::ItemInfo LibCommonService::getImgInfoByPath(QString path)
{
    QMutexLocker locker(&m_mutex);
    QMap<QString, imageViewerSpace::ItemInfo> m_tempInfoMap = m_allInfoMap;
    if (!m_tempInfoMap.contains(path)) {
        qWarning() << "Image info not found for path:" << path;
    }
    return m_tempInfoMap[path];
}

/*void CommonService::setImgInfoByPat(QString path, imageViewerSpace::ItemInfo itemInfo)
{
    m_allInfoMap[path] = itemInfo;
}*/

void LibCommonService::reName(const QString &oldPath, const QString &newPath)
{
    qDebug() << "Renaming image from" << oldPath << "to" << newPath;
    QMutexLocker locker(&m_mutex);
    imageViewerSpace::ItemInfo info = m_allInfoMap[oldPath];
    info.path = newPath;
    m_allInfoMap[newPath] = info;
    m_allInfoMap.remove(oldPath);
    emit ImageEngine::instance()->sigOneImgReady(oldPath, info);
}

void LibCommonService::slotSetImgInfoByPath(QString path, imageViewerSpace::ItemInfo itemInfo)
{
    qDebug() << "Setting image info for path:" << path;
    QMutexLocker locker(&m_mutex);
    m_allInfoMap[path] = itemInfo;
    emit ImageEngine::instance()->sigOneImgReady(path, itemInfo);
}

LibCommonService::LibCommonService(QObject *parent) : QObject(parent)
{
    qDebug() << "LibCommonService constructor called";
    qApp->installEventFilter(this);
}

bool LibCommonService::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (dynamic_cast<QMouseEvent *>(event)->button() == Qt::RightButton) {
            qDebug() << "Right mouse button pressed";
            emit sigRightMousePress();
        }
    }
    return QObject::eventFilter(obj, event);
}
