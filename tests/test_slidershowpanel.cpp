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

TEST_F(gtestview, slider_test)
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
    panel->resize(1280, 1080);
    panel->showNormal();
    panel->showFullScreen();

    panel->startSlideShow(info);
    QTest::qWait(40000);
    info.paths = QStringList(path);

    panel->startSlideShow(info);

    QTest::mouseMove(panel, QPoint(1000, 1075), 1000);
    QTest::mouseMove(panel, QPoint(200, 500), 1000);
    QTest::mouseMove(panel, QPoint(20, 20), 1000);
    QTest::mouseDClick(panel, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20), 1000);

    QAction menuAction;

    menuAction.setProperty("MenuID", LibSlideShowPanel::IdPlay);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(400);

    menuAction.setProperty("MenuID", LibSlideShowPanel:: IdPause);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(400);

    menuAction.setProperty("MenuID", LibSlideShowPanel::IdPlayOrPause);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(400);


    menuAction.setProperty("MenuID", LibSlideShowPanel::IdStopslideshow);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(400);

    panel->onSingleAnimationEnd();
    panel->onESCKeyStopSlide();
    panel->onShowContinue();
    panel->onShowPrevious();
    panel->onShowNext();
    panel->onCustomContextMenuRequested();

    panel->slideshowbottombar->showContinue();
    panel->slideshowbottombar->showNext();
    panel->slideshowbottombar->showPrevious();
    panel->slideshowbottombar->showCancel();
    panel->slideshowbottombar->showPause();

    panel->deleteLater();
    panel = nullptr;
}
TEST_F(gtestview, SlideShowBottomBar_test)
{
    SlideShowBottomBar *bar = new SlideShowBottomBar();

    bar->onPreButtonClicked();
    bar->isStop = true;
    bar->onPlaypauseButtonClicked();
    bar->isStop = false;
    bar->onPlaypauseButtonClicked();
    bar->onUpdatePauseButton();
    bar->onInitSlideShowButton();
    bar->onNextButtonClicked();
    bar->onCancelButtonClicked();

    bar->deleteLater();
    bar = nullptr;
}
