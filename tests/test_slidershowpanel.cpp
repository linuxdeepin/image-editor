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
#include "gtestview.h"
#include "slideshow/slideshowpanel.h"

TEST_F(gtestview, SlideShowBottomBar)
{
    QStringList list;
    list << QApplication::applicationDirPath() + "/gif.gif";
    list << QApplication::applicationDirPath() + "/gif2.gif";
    list << QApplication::applicationDirPath() + "/ico.ico";
    list << QApplication::applicationDirPath() + "/jpg.jpg";
    list << QApplication::applicationDirPath() + "/png.png";
    list << QApplication::applicationDirPath() + "/mng.mng";
    list << QApplication::applicationDirPath() + "/svg1.svg";
    list << QApplication::applicationDirPath() + "/svg2.svg";
    list << QApplication::applicationDirPath() + "/svg3.svg";
    QString path = QApplication::applicationDirPath() + "/gif.gif";

    QWidget *widgetParent = new QWidget();
    ViewInfo info;
    info.fullScreen = false;
    info.lastPanel = widgetParent;
    info.path = path;
    info.paths = list;
    info.viewMainWindowID = 0;

    LibSlideShowPanel *panel = new LibSlideShowPanel();
    panel->showNormal();
    panel->showFullScreen();

    panel->startSlideShow(info);

    info.paths = QStringList(path);

    panel->startSlideShow(info);

    QTest::qWait(1000);
    panel->deleteLater();
    panel = nullptr;
}

