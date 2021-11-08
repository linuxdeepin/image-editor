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
#include "accessibility/ac-desktop-define.h"
#include <DGuiApplicationHelper>
#include "imageviewer.h"
#include "imageengine.h"
#include "service/imagedataservice.h"
#include "viewpanel/navigationwidget.h"
#define  private public
#include "viewpanel/viewpanel.h"


TEST_F(gtestview, cp2Image)
{
    for (int i = 0; i < 200; i++) {
        QFile::copy(":/jpg.jpg", QApplication::applicationDirPath() + "/test/jpg" + QString::number(i) + ".jpg");
        QFile(QApplication::applicationDirPath() + "/test/jpg" + QString::number(i) + ".jpg").setPermissions(\
                                                                                                             QFile::WriteUser | QFile::ReadUser | QFile::WriteOther | \
                                                                                                             QFile::ReadOther | QFile::ReadGroup | QFile::WriteGroup);
    }
}

//view panel
TEST_F(gtestview, LibViewPanel)
{
    //初始化
    LibViewPanel *panel = new LibViewPanel();
    panel->loadImage(QApplication::applicationDirPath() + "/gif.gif", {QApplication::applicationDirPath() + "/gif.gif",
                                                                       QApplication::applicationDirPath() + "/tif.tif",
                                                                       QApplication::applicationDirPath() + "/jpg.jpg",
                                                                       QApplication::applicationDirPath() + "/jpg.jpg",
                                                                       QApplication::applicationDirPath() + "/svg.svg"
                                                                      });
    panel->initFloatingComponent();
    panel->show();

    //键盘与鼠标事件
    QTestEventList e;

    //动态图
    e.addKeyClick(Qt::Key_Right, Qt::KeyboardModifier::NoModifier, 200);
    e.addKeyClick(Qt::Key_Left, Qt::KeyboardModifier::NoModifier, 200);
    e.addKeyClick(Qt::Key_Up, Qt::KeyboardModifier::NoModifier, 200);
    e.addKeyClick(Qt::Key_Plus, Qt::KeyboardModifier::ControlModifier, 200);
    e.addKeyClick(Qt::Key_0, Qt::KeyboardModifier::ControlModifier, 200);
    e.addKeyClick(Qt::Key_Minus, Qt::KeyboardModifier::ControlModifier, 200);
    e.addKeyClick(Qt::Key_Down, Qt::KeyboardModifier::NoModifier, 200);
    e.addKeyClick(Qt::Key_Escape, Qt::KeyboardModifier::NoModifier, 200);
    e.addDelay(500);

    //多页图
    e.addKeyClick(Qt::Key_Right, Qt::KeyboardModifier::NoModifier, 200);
    e.addKeyClick(Qt::Key_Up, Qt::KeyboardModifier::NoModifier, 200);
    e.addKeyClick(Qt::Key_Plus, Qt::KeyboardModifier::ControlModifier, 200);
    e.addKeyClick(Qt::Key_0, Qt::KeyboardModifier::ControlModifier, 200);
    e.addKeyClick(Qt::Key_Minus, Qt::KeyboardModifier::ControlModifier, 200);
    e.addKeyClick(Qt::Key_Down, Qt::KeyboardModifier::NoModifier, 200);
    e.addKeyClick(Qt::Key_Escape, Qt::KeyboardModifier::NoModifier, 200);
    e.addDelay(500);

    e.simulate(panel);

    panel->deleteLater();
    panel = nullptr;
    QTest::qWait(500);

}
//view panel
TEST_F(gtestview, LibViewPanel1)
{
    //初始化
    LibViewPanel *panel = new LibViewPanel();
    panel->loadImage(QApplication::applicationDirPath() + "/gif.gif", {QApplication::applicationDirPath() + "/gif.gif",
                                                                       QApplication::applicationDirPath() + "/tif.tif",
                                                                       QApplication::applicationDirPath() + "/jpg.jpg",
                                                                       QApplication::applicationDirPath() + "/jpg.jpg",
                                                                       QApplication::applicationDirPath() + "/svg.svg"
                                                                      });
    panel->initFloatingComponent();
    panel->show();
//    //基本函数遍历
    panel->toggleFullScreen();
    panel->slotBottomMove();
    panel->toggleFullScreen();
    panel->slotBottomMove();

    panel->deleteLater();
    panel = nullptr;
    QTest::qWait(500);
}
TEST_F(gtestview, LibViewPanel_ocr)
{
    LibViewPanel *panel = new LibViewPanel();
    panel->loadImage(QApplication::applicationDirPath() + "/gif.gif", {QApplication::applicationDirPath() + "/gif.gif",
                                                                       QApplication::applicationDirPath() + "/tif.tif",
                                                                       QApplication::applicationDirPath() + "/jpg.jpg",
                                                                       QApplication::applicationDirPath() + "/jpg.jpg",
                                                                       QApplication::applicationDirPath() + "/svg.svg"
                                                                      });
    panel->getBottomtoolbarButton(imageViewerSpace::ButtonType::ButtonTypeOcr);
    panel->slotOcrPicture();
    panel->deleteLater();
    panel = nullptr;
}
TEST_F(gtestview, LibViewPanel_startChooseFileDialog1)
{
    QWidget *widget = new QWidget();
    //初始化
    LibViewPanel *panel = new LibViewPanel(nullptr, widget);
//    panel->backImageView(QApplication::applicationDirPath() + "/svg.svg");
    panel->initSlidePanel();
    panel->resetBottomToolbarGeometry(true);
    panel->resetBottomToolbarGeometry(false);
    panel->slotRotateImage(90);
    QTest::qWait(3000);
    panel->slotRotateImage(-90);
    QTest::qWait(3000);
}
TEST_F(gtestview, LibViewPanel_Menu)
{
//    //键盘与鼠标事件
//    QTestEventList e;

    LibViewPanel *panel = new LibViewPanel();
    panel->loadImage(QApplication::applicationDirPath() + "/gif.gif", {QApplication::applicationDirPath() + "/gif.gif",
                                                                       QApplication::applicationDirPath() + "/tif.tif",
                                                                       QApplication::applicationDirPath() + "/jpg.jpg",
                                                                       QApplication::applicationDirPath() + "/jpg.jpg",
                                                                       QApplication::applicationDirPath() + "/svg.svg"
                                                                      });
    //菜单
    //还剩IdPrint会崩
    QAction menuAction;

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdFullScreen);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdRename);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdStartSlideShow);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    //add new
    QTest::keyClick(panel, Qt::Key_Escape, Qt::KeyboardModifier::NoModifier, 200);
    QTest::qWait(200);
//    e.clear();
//    e.addKeyClick(Qt::Key_Escape, Qt::KeyboardModifier::NoModifier, 200);
//    e.simulate(panel->m_sliderPanel);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdCopy);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdMoveToTrash);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);
    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdMoveToTrash);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdShowNavigationWindow);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdHideNavigationWindow);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);
    DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::DarkType);
    QTest::qWait(500);
    DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::LightType);
    QTest::qWait(500);


    QTest::keyClick(panel->m_sliderPanel, Qt::Key_Escape, Qt::KeyboardModifier::NoModifier, 200);
    QTest::qWait(200);

    //add new
    QTest::mousePress(panel, Qt::LeftButton, Qt::NoModifier, QPoint(0, 0), 300);
    QTest::mouseMove(panel, QPoint(20, 20), 300);
    QTest::mouseRelease(panel, Qt::LeftButton, Qt::NoModifier, QPoint(200, 1020), 300);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdRotateClockwise);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdRotateCounterclockwise);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdSetAsWallpaper);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdDisplayInFileManager);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdImageInfo);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdOcr);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(500);

    menuAction.setProperty("MenuID", imageViewerSpace::NormalMenuItemId::IdMoveToTrash);
    panel->onMenuItemClicked(&menuAction);
    QTest::qWait(200);

    //ImageGraphicsView
    panel->loadImage("", {});

    auto view = panel->m_view;
    view->clear();
    view->setImage(QApplication::applicationDirPath() + "/svg2.svg", QImage());

    QTest::qWait(500);
}



