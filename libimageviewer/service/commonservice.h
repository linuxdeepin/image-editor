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
#ifndef COMMONSERVICE_H
#define COMMONSERVICE_H

#include <QObject>
#include <QStandardPaths>
#include <QDir>
#include <QMap>
#include <QMutex>
#include "image-viewer_global.h"

class LibCommonService : public QObject
{
    Q_OBJECT
public:
    static LibCommonService *instance();

    //设置图片展示类型，看图，相册
    void setImgViewerType(imageViewerSpace::ImgViewerType type);
    imageViewerSpace::ImgViewerType getImgViewerType();
    //设置缩略图保存路径
    void setImgSavePath(QString path);
    QString getImgSavePath();

    imageViewerSpace::ItemInfo getImgInfoByPath(QString path);
    //不发数据更新信号的保存信息
    //void setImgInfoByPat(QString path, imageViewerSpace::ItemInfo itemInfo);

    //重命名更新缓存
    void reName(const QString &oldPath, const QString &newPath);

public:
    QStringList m_listAllPath;//全部需要加载的
    QStringList m_noLoadingPath;//没有加载的地址
    QStringList m_listLoaded;//已经加载的

signals:
    void sigRightMousePress();
public slots:
    //有新的图片加载上来,保存制作好的图片信息
    void slotSetImgInfoByPath(QString path, imageViewerSpace::ItemInfo itemInfo);
private:
    explicit LibCommonService(QObject *parent = nullptr);
    bool eventFilter(QObject *obj, QEvent *event);
private:
    static LibCommonService *m_commonService;
    QMutex m_mutex;
    imageViewerSpace::ImgViewerType m_imgViewerType = imageViewerSpace::ImgViewerTypeNull;
    QString       m_imgSavePath;
    QMap<QString, imageViewerSpace::ItemInfo> m_allInfoMap;//图片所有信息map


};

#endif // COMMONSERVICE_H
