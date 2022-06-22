/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     LiuMingHang <liuminghang@uniontech.com>
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
#ifndef GRAPHICSMOVIEITEM_H
#define GRAPHICSMOVIEITEM_H

#include <QGraphicsPixmapItem>
#include <QPointer>
#include <QMovie>
class QMovie;
class LibGraphicsMovieItem : public QGraphicsPixmapItem, QObject
{
public:
    explicit LibGraphicsMovieItem(const QString &fileName, const QString &suffix = nullptr, QGraphicsItem *parent = nullptr);
    ~LibGraphicsMovieItem();
    bool isValid() const;
    void start();
    void stop();

private:
    QPointer<QMovie> m_movie;
};

class LibGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    explicit LibGraphicsPixmapItem(const QPixmap &pixmap);
    ~LibGraphicsPixmapItem() override;

    void setPixmap(const QPixmap &pixmap);
protected:
    //自绘函数
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
    QPair<qreal, QPixmap> cachePixmap;
};

#endif // GRAPHICSMOVIEITEM_H
