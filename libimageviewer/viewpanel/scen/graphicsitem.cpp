// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "graphicsitem.h"

#include <QDebug>
#include <QPainter>

LibGraphicsMovieItem::LibGraphicsMovieItem(const QString &fileName, const QString &suffix, QGraphicsItem *parent)
    : QGraphicsPixmapItem(fileName, parent)
{
    Q_UNUSED(suffix);
    m_movie = new QMovie(fileName);
    QObject::connect(m_movie, &QMovie::frameChanged, this, [ = ] {
        if (m_movie.isNull()) return;
        setPixmap(m_movie->currentPixmap());
    });
    //自动执行播放
    m_movie->start();
}

LibGraphicsMovieItem::~LibGraphicsMovieItem()
{
    // Prepares the item for a geometry change. Call this function
    // before changing the bounding rect of an item to keep
    // QGraphicsScene's index up to date.
    // If not doing this, it may crash
    prepareGeometryChange();

    m_movie->stop();
    m_movie->deleteLater();
    m_movie = nullptr;
}

/*!
 * \brief GraphicsMovieItem::isValid
 * There is a bug with QMovie::isValid() that is event if file's format not
 * supported this function still return true.
 * \return
 */
bool LibGraphicsMovieItem::isValid() const
{
    return m_movie->frameCount() > 1;
}

void LibGraphicsMovieItem::start()
{
    m_movie->start();
}

void LibGraphicsMovieItem::stop()
{
    m_movie->stop();
}


LibGraphicsPixmapItem::LibGraphicsPixmapItem(const QPixmap &pixmap)
    : QGraphicsPixmapItem(pixmap, nullptr)
{

}

LibGraphicsPixmapItem::~LibGraphicsPixmapItem()
{
    prepareGeometryChange();
}

void LibGraphicsPixmapItem::setPixmap(const QPixmap &pixmap)
{
    cachePixmap = qMakePair(cachePixmap.first, pixmap);
    QGraphicsPixmapItem::setPixmap(pixmap);
}

void LibGraphicsPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    const QTransform ts = painter->transform();

    if (ts.type() == QTransform::TxScale && ts.m11() < 1) {
        QPixmap currentPixmap = pixmap();
        if (currentPixmap.width() < 10000 && currentPixmap.height() < 10000) {
            painter->setRenderHint(QPainter::SmoothPixmapTransform,
                                   (transformationMode() == Qt::SmoothTransformation));

            Q_UNUSED(option);
            Q_UNUSED(widget);

            QPixmap pixmap;

            if (qIsNull(cachePixmap.first - ts.m11())) {
                pixmap = cachePixmap.second;
            } else {
                pixmap = currentPixmap.transformed(painter->transform(), transformationMode());
                cachePixmap = qMakePair(ts.m11(), pixmap);
            }

            pixmap.setDevicePixelRatio(painter->device()->devicePixelRatioF());
            painter->resetTransform();
            painter->drawPixmap(offset() + QPointF(ts.dx(), ts.dy()), pixmap);
            painter->setTransform(ts);
        } else {
            QGraphicsPixmapItem::paint(painter, option, widget);
        }
    } else {
        QGraphicsPixmapItem::paint(painter, option, widget);
    }
}


