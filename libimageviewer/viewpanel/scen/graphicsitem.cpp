// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "graphicsitem.h"

#include <QDebug>
#include <QPainter>

#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

LibGraphicsMovieItem::LibGraphicsMovieItem(const QString &fileName, const QString &suffix, QGraphicsItem *parent)
    : QGraphicsPixmapItem(fileName, parent)
{
    qDebug() << "Initializing LibGraphicsMovieItem with file:" << fileName;
    Q_UNUSED(suffix);

    setTransformationMode(Qt::SmoothTransformation);

    m_movie = new QMovie(fileName);
    QObject::connect(m_movie, &QMovie::frameChanged, this, [=] {
        if (m_movie.isNull()) {
            qWarning() << "Movie object is null during frame change";
            return;
        }
        setPixmap(m_movie->currentPixmap());
    });
    //自动执行播放
    m_movie->start();
    qDebug() << "Movie started for file:" << fileName;
}

LibGraphicsMovieItem::~LibGraphicsMovieItem()
{
    qDebug() << "Destroying LibGraphicsMovieItem";
    // Prepares the item for a geometry change. Call this function
    // before changing the bounding rect of an item to keep
    // QGraphicsScene's index up to date.
    // If not doing this, it may crash
    prepareGeometryChange();

    m_movie->stop();
    m_movie->deleteLater();
    m_movie = nullptr;
    qDebug() << "Movie stopped and resources cleaned up";
}

/*!
 * \brief GraphicsMovieItem::isValid
 * There is a bug with QMovie::isValid() that is event if file's format not
 * supported this function still return true.
 * \return
 */
bool LibGraphicsMovieItem::isValid() const
{
    bool valid = m_movie->frameCount() > 1;
    qDebug() << "Checking movie validity, frame count:" << m_movie->frameCount() << "is valid:" << valid;
    return valid;
}

void LibGraphicsMovieItem::start()
{
    qDebug() << "Starting movie playback";
    m_movie->start();
}

void LibGraphicsMovieItem::stop()
{
    qDebug() << "Stopping movie playback";
    m_movie->stop();
}

LibGraphicsPixmapItem::LibGraphicsPixmapItem(const QPixmap &pixmap)
    : QGraphicsPixmapItem(pixmap, nullptr)
{
    qDebug() << "Initializing LibGraphicsPixmapItem with pixmap size:" << pixmap.size();
}

LibGraphicsPixmapItem::~LibGraphicsPixmapItem()
{
    qDebug() << "Destroying LibGraphicsPixmapItem";
    prepareGeometryChange();
}

void LibGraphicsPixmapItem::setPixmap(const QPixmap &pixmap)
{
    qDebug() << "Setting new pixmap with size:" << pixmap.size();
    cachePixmap = qMakePair(cachePixmap.first, pixmap);
    QGraphicsPixmapItem::setPixmap(pixmap);
}

void LibGraphicsPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    const QTransform ts = painter->transform();

    if (ts.type() == QTransform::TxScale && ts.m11() < 1) {
        QPixmap currentPixmap = pixmap();
        if (currentPixmap.width() < 10000 && currentPixmap.height() < 10000) {
            qDebug() << "Painting scaled pixmap, scale factor:" << ts.m11();
            painter->setRenderHint(QPainter::SmoothPixmapTransform, (transformationMode() == Qt::SmoothTransformation));

            Q_UNUSED(option);
            Q_UNUSED(widget);

            QPixmap pixmap;

            if (qIsNull(cachePixmap.first - ts.m11())) {
                qDebug() << "Using cached pixmap";
                pixmap = cachePixmap.second;
            } else {
                qDebug() << "Transforming pixmap for new scale";
                pixmap = currentPixmap.transformed(painter->transform(), transformationMode());
                cachePixmap = qMakePair(ts.m11(), pixmap);
            }

            pixmap.setDevicePixelRatio(painter->device()->devicePixelRatioF());
            painter->resetTransform();
            painter->drawPixmap(offset() + QPointF(ts.dx(), ts.dy()), pixmap);
            painter->setTransform(ts);
        } else {
            qWarning() << "Pixmap too large for optimized painting, using default paint method";
            QGraphicsPixmapItem::paint(painter, option, widget);
        }
    } else {
        QGraphicsPixmapItem::paint(painter, option, widget);
    }
}

LibGraphicsMaskItem::LibGraphicsMaskItem(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    qDebug() << "Initializing LibGraphicsMaskItem";
    onThemeChange(DGuiApplicationHelper::instance()->themeType());
    conn = QObject::connect(DGuiApplicationHelper::instance(),
                            &DGuiApplicationHelper::themeTypeChanged,
                            [this](DGuiApplicationHelper::ColorType themeType) { 
                                qDebug() << "Theme changed to:" << (themeType == DGuiApplicationHelper::DarkType ? "Dark" : "Light");
                                this->onThemeChange(themeType); 
                            });
}

LibGraphicsMaskItem::~LibGraphicsMaskItem()
{
    qDebug() << "Destroying LibGraphicsMaskItem";
    QObject::disconnect(conn);
}

void LibGraphicsMaskItem::onThemeChange(int theme)
{
    qDebug() << "Updating mask color for theme:" << (theme == DGuiApplicationHelper::DarkType ? "Dark" : "Light");
    QColor maskColor;
    if (DGuiApplicationHelper::ColorType::DarkType == theme) {
        maskColor = QColor(Qt::black);
        maskColor.setAlphaF(0.6);
    } else {
        maskColor = QColor(Qt::white);
        maskColor.setAlphaF(0.6);
    }

    QPen curPen = pen();
    curPen.setColor(maskColor);
    setPen(curPen);
    setBrush(maskColor);
    update();
    qDebug() << "Mask color updated with alpha:" << maskColor.alphaF();
}
