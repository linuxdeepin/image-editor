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
#include "viewpanel.h"

#include <QVBoxLayout>
#include <QShortcut>
#include <QFileInfo>
#include <QDBusInterface>
#include <QGuiApplication>
#include <QScreen>
#include <QApplication>
#include <QStringList>
#include <QDesktopWidget>

#include <DDesktopServices>
#include <DMenu>
#include <DFileDialog>

#include "contents/bottomtoolbar.h"
#include "navigationwidget.h"
#include "lockwidget.h"
#include "thumbnailwidget.h"

#include "unionimage/imageutils.h"
#include "unionimage/baseutils.h"
#include "unionimage/pluginbaseutils.h"
#include "unionimage/unionimage.h"
#include "imageengine.h"
#include "widgets/printhelper.h"
#include "contents/imageinfowidget.h"
#include "widgets/extensionpanel.h"
#include "widgets/toptoolbar.h"
#include "widgets/renamedialog.h"
#include "service/ocrinterface.h"
#include "slideshow/slideshowpanel.h"
#include "service/configsetter.h"

const QString IMAGE_TMPPATH =   QDir::homePath() +
                                "/.config/deepin/deepin-image-viewer/";

const int BOTTOM_TOOLBAR_HEIGHT = 80;   //底部工具看高
const int BOTTOM_SPACING = 10;          //底部工具栏与底部边缘距离
const int RT_SPACING = 20;
const int TOP_TOOLBAR_HEIGHT = 50;
const int DELAY_HIDE_CURSOR_INTERVAL = 3000;

using namespace imageViewerSpace;

bool compareByFileInfo(const QFileInfo &str1, const QFileInfo &str2)
{
    static QCollator sortCollator;
    sortCollator.setNumericMode(true);
    return sortCollator.compare(str1.baseName(), str2.baseName()) < 0;
}

QString ss(const QString &text, const QString &defaultValue)
{
    Q_UNUSED(text);
    //采用代码中快捷键不使用配置文件快捷键
    // QString str = dApp->setter->value(SHORTCUTVIEW_GROUP, text, defaultValue).toString();
    QString str = defaultValue;
    str.replace(" ", "");
    return defaultValue;
}

LibViewPanel::LibViewPanel(AbstractTopToolbar *customToolbar, QWidget *parent)
    : QFrame(parent)
    , m_topToolbar(customToolbar)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);
    m_stack = new DStackedWidget(this);
    layout->addWidget(m_stack);

    m_view = new LibImageGraphicsView(this);
    m_stack->addWidget(m_view);

    //m_bottomToolbar的父为主窗口,就不会出现右键菜单
    m_bottomToolbar = new LibBottomToolbar(dynamic_cast<QWidget *>(this->parent()));

    setContextMenuPolicy(Qt::CustomContextMenu);

    initRightMenu();
    initFloatingComponent();
    initTopBar();
    initShortcut();
    initLockPanel();
    initThumbnailWidget();
    initConnect();

    setAcceptDrops(true);
//    initExtensionPanel();

    QObject::connect(m_view, &LibImageGraphicsView::currentThumbnailChanged, m_bottomToolbar, &LibBottomToolbar::onThumbnailChanged);
    QObject::connect(m_view, &LibImageGraphicsView::gestureRotate, this, &LibViewPanel::slotRotateImage);
}

LibViewPanel::~LibViewPanel()
{
}

void LibViewPanel::loadImage(const QString &path, QStringList paths)
{
    QFileInfo info(path);
    m_topToolbar->setMiddleContent(info.fileName());
    //展示图片
//    m_view->setImage(path);
//    m_view->resetTransform();
//    m_stack->setCurrentWidget(m_view);
    //刷新工具栏,如果paths不含有path,则添加进入paths
    if (!paths.contains(path)) {
        paths << path;
    }
    m_bottomToolbar->setAllFile(path, paths);
    m_currentPath = path;
    //刷新收藏按钮
    emit ImageEngine::instance()->sigUpdateCollectBtn();
    //重置底部工具栏位置与大小
    resetBottomToolbarGeometry(true);

    m_dirWatcher->removePaths(m_dirWatcher->directories());
    m_dirWatcher->addPath(QFileInfo(path).dir().path());
}

void LibViewPanel::initConnect()
{
    //缩略图列表，单机打开图片
    connect(m_bottomToolbar, &LibBottomToolbar::openImg, this, &LibViewPanel::openImg);

    connect(m_view, &LibImageGraphicsView::imageChanged, this, [ = ](QString path) {
        emit imageChanged(path);
        // Pixmap is cache in thread, make sure the size would correct after
        // cache is finish
        //暂时屏蔽，这里存在疑问，放开会导致每次切换图片，1:1高亮
        // m_view->autoFit();
    });


    //旋转信号
    connect(m_bottomToolbar, &LibBottomToolbar::rotateClockwise, this, [ = ] {
        this->slotRotateImage(-90);
    });

    connect(m_bottomToolbar, &LibBottomToolbar::rotateCounterClockwise, this, [ = ] {
        this->slotRotateImage(90);
    });

    //适应窗口和适应图片按钮
    connect(m_bottomToolbar, &LibBottomToolbar::resetTransform, this, &LibViewPanel::slotResetTransform);

    //删除后需要重新布局
    connect(m_bottomToolbar, &LibBottomToolbar::removed, this, [ = ] {
        //重新布局
        this->resetBottomToolbarGeometry(true);
    }, Qt::DirectConnection);

    //切换图片需要保存旋转
    connect(m_bottomToolbar, &LibBottomToolbar::sigRotateSave, m_view, &LibImageGraphicsView::slotRotatePixCurrent);

    //适应窗口的状态更新
    connect(m_view, &LibImageGraphicsView::checkAdaptScreenBtn, m_bottomToolbar, &LibBottomToolbar::checkAdaptImageBtn);
    connect(m_view, &LibImageGraphicsView::disCheckAdaptScreenBtn,  m_bottomToolbar, &LibBottomToolbar::disCheckAdaptScreenBtn);
    connect(m_view, &LibImageGraphicsView::checkAdaptImageBtn, m_bottomToolbar, &LibBottomToolbar::checkAdaptImageBtn);
    connect(m_view, &LibImageGraphicsView::disCheckAdaptImageBtn, m_bottomToolbar, &LibBottomToolbar::disCheckAdaptImageBtn);

    connect(m_bottomToolbar, &LibBottomToolbar::sigOcr, this, &LibViewPanel::slotOcrPicture);

    connect(m_view, &LibImageGraphicsView::sigImageOutTitleBar, m_topToolbar, &AbstractTopToolbar::setTitleBarTransparent);

    connect(m_view, &LibImageGraphicsView::sigMouseMove, this, &LibViewPanel::slotBottomMove);

    connect(ImageEngine::instance(), &ImageEngine::sigOneImgReady, this, &LibViewPanel::slotOneImgReady);

    connect(m_view, &LibImageGraphicsView::UpdateNavImg, this, [ = ]() {
        m_nav->setImage(m_view->image());
        m_nav->setRectInImage(m_view->visibleImageRect());

//二指放大会触发信号，导致窗口隐藏，这里下面存在问题
//        //正在滑动缩略图的时候不再显示
//        if (m_nav->isVisible()) {
//            m_nav->setVisible(false);
//        }
    });

    connect(m_view, &LibImageGraphicsView::sigFIleDelete, this, [ = ]() {
        this->updateMenuContent();
    });
    //增加双击全屏和退出全屏的功能
    connect(m_view, &LibImageGraphicsView::doubleClicked, this, [ = ]() {
        toggleFullScreen();
    });

    //上一页，下一页信号连接
    connect(m_view, &LibImageGraphicsView::previousRequested, this, &LibViewPanel::showPrevious);
    connect(m_view, &LibImageGraphicsView::nextRequested, this, &LibViewPanel::showNext);

    m_dirWatcher = new  QFileSystemWatcher(this);
    connect(m_dirWatcher, &QFileSystemWatcher::directoryChanged, this, &LibViewPanel::slotsDirectoryChanged);
}

void LibViewPanel::initTopBar()
{
    //防止在标题栏右键菜单会触发默认的和主窗口的发生
    if (m_topToolbar == nullptr) { //如果调用者没有指定有效的顶部栏，则使用内置方案
        m_topToolbar = new LibTopToolbar(false, dynamic_cast<QWidget *>(this->parent()));
    } else {
        m_topToolbar->setParent(dynamic_cast<QWidget *>(this->parent()));
    }
    m_topToolbar->resize(width(), 50);
    m_topToolbar->move(0, 0);
    m_topToolbar->setTitleBarTransparent(false);
}

void LibViewPanel::initOcr()
{
    if (!m_ocrInterface) {
        m_ocrInterface = new OcrInterface("com.deepin.Ocr", "/com/deepin/Ocr", QDBusConnection::sessionBus(), this);
    }
}

void LibViewPanel::initFloatingComponent()
{
    initScaleLabel();
    initNavigation();
}

void LibViewPanel::initScaleLabel()
{
    using namespace Libutils::base;
    DAnchors<DFloatingWidget> scalePerc = new DFloatingWidget(this);
    scalePerc->setBlurBackgroundEnabled(true);

    QHBoxLayout *layout = new QHBoxLayout();
    scalePerc->setLayout(layout);
    DLabel *label = new DLabel();
    layout->addWidget(label);
    scalePerc->setAttribute(Qt::WA_TransparentForMouseEvents);
    scalePerc.setAnchor(Qt::AnchorHorizontalCenter, this, Qt::AnchorHorizontalCenter);
    scalePerc.setAnchor(Qt::AnchorBottom, this, Qt::AnchorBottom);
    scalePerc.setBottomMargin(75 + 14);
    label->setAlignment(Qt::AlignCenter);
//    scalePerc->setFixedSize(82, 48);
    scalePerc->setFixedWidth(90 + 10);
    scalePerc->setFixedHeight(40 + 10);
    scalePerc->adjustSize();
    label->setText("100%");
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T6);
    scalePerc->hide();

    QTimer *hideT = new QTimer(this);
    hideT->setSingleShot(true);
    connect(hideT, &QTimer::timeout, scalePerc, &DLabel::hide);

    connect(m_view, &LibImageGraphicsView::scaled, this, [ = ](qreal perc) {
        label->setText(QString("%1%").arg(int(perc)));
    });
    connect(m_view, &LibImageGraphicsView::showScaleLabel, this, [ = ]() {
        scalePerc->show();
        hideT->start(1000);
    });
}

void LibViewPanel::initNavigation()
{
    m_nav = new NavigationWidget(this);
    m_nav.setBottomMargin(100);
    m_nav.setLeftMargin(10);
    m_nav.setAnchor(Qt::AnchorLeft, this, Qt::AnchorLeft);
    m_nav.setAnchor(Qt::AnchorBottom, this, Qt::AnchorBottom);

    connect(this, &LibViewPanel::imageChanged, this, [ = ](const QString & path) {
        Q_UNUSED(path)
        //BUG#93145 去除对path的判断，直接隐藏导航窗口
        m_nav->setVisible(false);
        m_nav->setImage(m_view->image());
        //转移到中心位置
//        m_bottomToolbar->thumbnailMoveCenterWidget();
    });

    connect(m_nav, &NavigationWidget::requestMove, [this](int x, int y) {
        m_view->centerOn(x, y);
    });
    connect(m_view, &LibImageGraphicsView::transformChanged, this, [ = ]() {
        //如果stackindex不为2，全屏会出现导航窗口
        //如果是正在移动的情况，将不会出现导航栏窗口
        if (m_stack->currentWidget() == m_view) {
            m_nav->setVisible((! m_nav->isAlwaysHidden() && ! m_view->isWholeImageVisible()));
            m_nav->setRectInImage(m_view->visibleImageRect());
        } else {
            m_nav->setVisible(false);
        }
    });
}

void LibViewPanel::initRightMenu()
{
    //初始化时设置所有菜单项都显示
    m_menuItemDisplaySwitch.set();

    if (!m_menu) {
        m_menu = new DMenu(this);
    }
    QShortcut *ctrlm = new QShortcut(QKeySequence("Ctrl+M"), this);
    ctrlm->setContext(Qt::WindowShortcut);
    connect(ctrlm, &QShortcut::activated, this, [ = ] {
        this->customContextMenuRequested(cursor().pos());
    });

    connect(this, &LibViewPanel::customContextMenuRequested, this, [ = ] {
        updateMenuContent();
        m_menu->popup(QCursor::pos());
    });
    connect(m_menu, &DMenu::triggered, this, &LibViewPanel::onMenuItemClicked);
}

void LibViewPanel::initExtensionPanel()
{
    if (!m_info) {
        m_info = new LibImageInfoWidget("", "", this);
        m_info->hide();
    }
    m_info->setImagePath(m_bottomToolbar->getCurrentItemInfo().path);
    if (!m_extensionPanel) {
        m_extensionPanel = new ExtensionPanel(this);
        connect(m_info, &LibImageInfoWidget::extensionPanelHeight, m_extensionPanel, &ExtensionPanel::updateRectWithContent);
        connect(m_view, &LibImageGraphicsView::clicked, this, [ = ] {
            this->m_extensionPanel->hide();
            this->m_info->show();
        });
    }
}

void LibViewPanel::updateMenuContent(QString path)
{
    //判断是否为相册调用
    bool isAlbum = false;
    if (ImgViewerType::ImgViewerTypeAlbum == LibCommonService::instance()->getImgViewerType()) {
        isAlbum = true;
    }
    if (!window()->isFullScreen()) {
        resetBottomToolbarGeometry(true);
    }

    if (m_menu) {
        m_menu->clear();
        qDeleteAll(this->actions());

        imageViewerSpace::ItemInfo ItemInfo = m_bottomToolbar->getCurrentItemInfo();

        bool isPic = !ItemInfo.image.isNull();
        if (!isPic) {
            isPic = !m_view->image().isNull();//当前视图是否是图片
        }

        QString currentPath;
        if (path.isEmpty()) {
            currentPath = ItemInfo.path;
        } else {
            currentPath = path;
        }
        if (currentPath.isEmpty()) {
            currentPath = m_currentPath;
        }
        QFileInfo info(currentPath);
        bool isReadable = info.isReadable() ; //是否可读
        qDebug() << QFileInfo(info.dir(), info.dir().path()).isWritable();
        //判断文件是否可写和文件目录是否可写
        bool isWritable = info.isWritable() && QFileInfo(info.dir(), info.dir().path()).isWritable(); //是否可写
//        bool isFile = info.isFile(); //是否存在
        bool isRotatable = ImageEngine::instance()->isRotatable(currentPath);//是否可旋转
        imageViewerSpace::PathType pathType = LibUnionImage_NameSpace::getPathType(currentPath);//路径类型
        imageViewerSpace::ImageType imageType = LibUnionImage_NameSpace::getImageType(currentPath);//图片类型

        //判断是否是损坏图片
        setCurrentWidget(currentPath);

        if (m_info) {
            m_info->setImagePath(currentPath);
        }
//        if (!isFile && !currentPath.isEmpty()) {
//            if (m_thumbnailWidget) {
//                m_stack->setCurrentWidget(m_thumbnailWidget);
//                //损坏图片不透明
//                emit m_view->sigImageOutTitleBar(false);
//                m_thumbnailWidget->setThumbnailImage(QPixmap::fromImage(ItemInfo.image));
//            }
//        } else if (isPic) {
//            m_stack->setCurrentWidget(m_view);
//            //判断下是否透明
//            m_view->titleBarControl();
//        } else if (!currentPath.isEmpty() && ItemInfo.pathType == pathType && ItemInfo.imageType == imageType) {
//            if (m_lockWidget) {
//                m_stack->setCurrentWidget(m_lockWidget);
//                //损坏图片不透明
//                emit m_view->sigImageOutTitleBar(false);
//            }
//        }

        //如果是图片，按钮恢复，否则按钮置灰
//        if (isPic) {
//            m_bottomToolbar->setPictureDoBtnClicked(true);
//        } else {
//            m_bottomToolbar->setPictureDoBtnClicked(false);
//        }

        if (imageViewerSpace::ImageTypeDamaged == imageType) {
            return;
        }
        DIconButton *AdaptImageButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypeAdaptImage);
        DIconButton *AdaptScreenButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypeAdaptScreen);
        //修复外部删除图片仍然能够使用适应图片和适应窗口的问题
        if (isPic && isReadable) {
            AdaptImageButton->setEnabled(true);
            AdaptScreenButton->setEnabled(true);
        } else {
            AdaptImageButton->setEnabled(false);
            AdaptScreenButton->setEnabled(false);
        }
        if (!isPic) {
            AdaptScreenButton->setChecked(isPic);
            AdaptImageButton->setChecked(isPic);
        }

        if (window()->isFullScreen()) {
            appendAction(IdExitFullScreen, QObject::tr("Exit fullscreen"), ss("Fullscreen", "F11"));
        } else {
            appendAction(IdFullScreen, QObject::tr("Fullscreen"), ss("Fullscreen", "F11"));
        }

        appendAction(IdPrint, QObject::tr("Print"), ss("Print", "Ctrl+P"));

        //ocr按钮,是否是动态图,todo
        DIconButton *OcrButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypeOcr);
        if (imageViewerSpace::ImageTypeDynamic != imageType && isPic && isReadable) {
            appendAction(IdOcr, QObject::tr("Extract text"), ss("Extract text", "Alt+O"));
            OcrButton->setEnabled(true);
        } else {
            OcrButton->setEnabled(false);
        }

        //如果图片数量大于0才能有幻灯片
        appendAction(IdStartSlideShow, QObject::tr("Slide show"), ss("Slide show", "F5"));
        //添加到相册
        if (isAlbum && isReadable) {
            emit ImageEngine::instance()->sigGetAlbumName(ItemInfo.path);
            //不管有无自定义相册，均需保留添加到相册
            m_menu->addSeparator();
            DMenu *am = new DMenu(tr("Add to album"));

            QAction *ac1 = new QAction(am);
            ac1->setProperty("MenuID", IdAddToAlbum);
            ac1->setText(tr("New album"));
            ac1->setData("Add to new album");
            ac1->setShortcut(QKeySequence("Ctrl+Shift+N"));
            am->addAction(ac1);
            am->addSeparator();
            if (!m_CustomAlbumName.isEmpty()) {
                for (auto iter = m_CustomAlbumName.begin(); iter != m_CustomAlbumName.end(); iter++) {
                    QAction *ac = new QAction(am);
                    ac->setProperty("MenuID", IdAddToAlbum);
                    ac->setText(fontMetrics().elidedText(QString(iter.key()).replace("&", "&&"), Qt::ElideMiddle, 200));
                    ac->setData(iter.key());
                    am->addAction(ac);
                    if (iter.value()) {
                        ac->setEnabled(false);
                    }
                }
            }
            m_menu->addMenu(am);
        }

        m_menu->addSeparator();
        if (isAlbum && isReadable) {
            appendAction(IdExport, tr("Export"), ss("Export", "Ctrl+E"));   //导出
        }
        if (isReadable) {
            appendAction(IdCopy, QObject::tr("Copy"), ss("Copy", "Ctrl+C"));
        }

        //如果程序有可读可写的权限,才能重命名,todo
        //20211019新增：安卓手机和苹果手机也不进行重命名
        if (isReadable && isWritable &&
                imageViewerSpace::PathTypeMTP != pathType &&
                imageViewerSpace::PathTypePTP != pathType &&
                imageViewerSpace::PathTypeAPPLE != pathType && !isAlbum) {
            appendAction(IdRename, QObject::tr("Rename"), ss("Rename", "F2"));
        }

        //apple phone的delete没有权限,保险箱无法删除,垃圾箱也无法删除,其他需要判断可读权限,todo
        //20211019新增：安卓手机也不进行删除
        DIconButton *TrashButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypeTrash);
        if ((imageViewerSpace::PathTypeAPPLE != pathType &&
                imageViewerSpace::PathTypeSAFEBOX != pathType &&
                imageViewerSpace::PathTypeRECYCLEBIN != pathType &&
                imageViewerSpace::PathTypeMTP != pathType &&
                imageViewerSpace::PathTypePTP != pathType &&
                isWritable && isReadable) || isAlbum) { //如果是相册，应当永久激活删除
            if (isAlbum) {
                appendAction(IdMoveToTrash, QObject::tr("Delete"), ss("Throw to trash", ""));
            } else {
                appendAction(IdMoveToTrash, QObject::tr("Delete"), ss("Throw to trash", "Delete"));
            }
            TrashButton->setEnabled(true);
        } else {
            TrashButton->setEnabled(false);
        }
        //IdRemoveFromAlbum
        if (isAlbum && m_isCustomAlbum && isReadable) {
            appendAction(IdRemoveFromAlbum, tr("Remove from album"), ss("Remove from album", ""));
        }
        m_menu->addSeparator();
        //fav
        if (isAlbum && isReadable) {
            if (m_isFav) {
                appendAction(IdRemoveFromFavorites, tr("Unfavorite"), ".");    //取消收藏
            } else {
                appendAction(IdAddToFavorites, tr("Favorite"), ".");       //收藏
            }
            m_menu->addSeparator();
        }

        //判断导航栏隐藏,需要添加一个当前是否有图片,todo
        if (isReadable && isPic && !m_view->isWholeImageVisible() && m_nav->isAlwaysHidden()) {
            appendAction(IdShowNavigationWindow, QObject::tr("Show navigation window"),
                         ss("Show navigation window", ""));
        } else if (isReadable && isPic && !m_view->isWholeImageVisible() && !m_nav->isAlwaysHidden()) {
            appendAction(IdHideNavigationWindow, QObject::tr("Hide navigation window"),
                         ss("Hide navigation window", ""));
        }

        //20211019修改：都可以转，但特殊位置不能执行写
        if (isRotatable && isWritable && isPic) {
            appendAction(IdRotateClockwise, QObject::tr("Rotate clockwise"), ss("Rotate clockwise", "Ctrl+R"));
            appendAction(IdRotateCounterclockwise, QObject::tr("Rotate counterclockwise"),
                         ss("Rotate counterclockwise", "Ctrl+Shift+R"));
            if (m_bottomToolbar) {
                m_bottomToolbar->setRotateBtnClicked(true);
            }
        } else {
            if (m_bottomToolbar) {
                m_bottomToolbar->setRotateBtnClicked(false);
            }

        }

        //需要判断图片是否支持设置壁纸,todo
        if (isPic && Libutils::image::imageSupportWallPaper(ItemInfo.path)) {
            appendAction(IdSetAsWallpaper, QObject::tr("Set as wallpaper"), ss("Set as wallpaper", "Ctrl+F9"));
        }
        if (isReadable) {
            appendAction(IdDisplayInFileManager, QObject::tr("Display in file manager"),
                         ss("Display in file manager", "Alt+D"));
            appendAction(IdImageInfo, QObject::tr("Image info"), ss("Image info", "Ctrl+I"));
        }
    }
}

void LibViewPanel::toggleFullScreen()
{
//    m_view->setFitState(false, false);
    if (window()->isFullScreen()) {
        showNormal();
        killTimer(m_hideCursorTid);
        m_hideCursorTid = 0;
        m_view->viewport()->setCursor(Qt::ArrowCursor);
    } else {
        showFullScreen();
        if (!m_menu || !m_menu->isVisible()) {
            m_view->viewport()->setCursor(Qt::BlankCursor);
        }
    }
}

void LibViewPanel::showFullScreen()
{
    m_isMaximized = window()->isMaximized();
    // Full screen then hide bars because hide animation depends on height()
    //加入动画效果，掩盖左上角展开的视觉效果，以透明度0-1显示。,时间为50ms

    //停止工具栏的动画
    if (m_bottomAnimation) {
        m_bottomAnimation->stop();
    }

    QPropertyAnimation *pAn = new QPropertyAnimation(window(), "windowOpacity");
    pAn->setDuration(50);
    pAn->setEasingCurve(QEasingCurve::Linear);
    pAn->setEndValue(1);
    pAn->setStartValue(0);
    pAn->start(QAbstractAnimation::DeleteWhenStopped);
    //增加切换全屏和默认大小下方工具栏的移动
//    connect(pAn, &QPropertyAnimation::destroyed, this, [ = ] {
//        slotBottomMove();
//    });

    window()->showFullScreen();
    m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);

}

void LibViewPanel::showNormal()
{
    //加入动画效果，掩盖左上角展开的视觉效果，以透明度0-1显示。
    //停止工具栏的动画
    if (m_bottomAnimation) {
        m_bottomAnimation->stop();
    }
    QPropertyAnimation *pAn = new QPropertyAnimation(window(), "windowOpacity");
    pAn->setDuration(50);
    pAn->setEasingCurve(QEasingCurve::Linear);
    pAn->setEndValue(1);
    pAn->setStartValue(0);
    pAn->start(QAbstractAnimation::DeleteWhenStopped);
    if (m_isMaximized) {
        window()->showNormal();
        window()->showMaximized();
    } else {
        window()->showNormal();
    }
    //增加切换全屏和默认大小下方工具栏的移动
    connect(pAn, &QPropertyAnimation::destroyed, this, [ = ] {
        m_bottomToolbar->move((width() - m_bottomToolbar->width()) / 2, height() - m_bottomToolbar->height() - 10);
        m_bottomToolbar->update();
    });
}

void LibViewPanel::appendAction(int id, const QString &text, const QString &shortcut)
{
    if (m_menu && m_menuItemDisplaySwitch.test(static_cast<size_t>(id))) {
        QAction *ac = new QAction(m_menu);
        addAction(ac);
        ac->setText(text);
        ac->setProperty("MenuID", id);
        ac->setShortcut(QKeySequence(shortcut));
        m_menu->addAction(ac);
    }
}

void LibViewPanel::setContextMenuItemVisible(imageViewerSpace::NormalMenuItemId id, bool visible)
{
    m_menuItemDisplaySwitch.set(id, visible);
    updateMenuContent();
}

void LibViewPanel::setWallpaper(const QImage &img)
{
    QThread *th1 = QThread::create([ = ]() {
        if (!img.isNull()) {
            QString path = "/tmp/DIVIMG.png";
            img.save("/tmp/DIVIMG.png", "png");
            //202011/12 bug54279
            {
                //设置壁纸代码改变，采用DBus,原方法保留
                if (/*!qEnvironmentVariableIsEmpty("FLATPAK_APPID")*/1) {
                    // gdbus call -e -d com.deepin.daemon.Appearance -o /com/deepin/daemon/Appearance -m com.deepin.daemon.Appearance.Set background /home/test/test.png
                    qDebug() << "SettingWallpaper: " << "flatpak" << path;
                    QDBusInterface interface("com.deepin.daemon.Appearance",
                                                 "/com/deepin/daemon/Appearance",
                                                 "com.deepin.daemon.Appearance");
//                    if (interface.isValid()) {
//                        //获取鼠标在的位置的桌面
//                        QString screenname = QGuiApplication::screenAt(QCursor::pos())->name();
//                        QDBusMessage reply = interface.call(QStringLiteral("SetMonitorBackground"), screenname, path);
//                        qDebug() << "SettingWallpaper: replay" << reply.errorMessage();
//                    }
                    if (interface.isValid()) {
                        QString screenname;

                        //判断环境是否是wayland
                        auto e = QProcessEnvironment::systemEnvironment();
                        QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
                        QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

                        bool isWayland = false;
                        if (XDG_SESSION_TYPE != QLatin1String("wayland") && !WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
                            isWayland = false;
                        } else {
                            isWayland = true;
                        }
                        //wayland下设置壁纸使用，2020/09/21
                        if (isWayland) {
                            QDBusInterface interfaceWayland("com.deepin.daemon.Display", "/com/deepin/daemon/Display", "com.deepin.daemon.Display");
                            screenname = qvariant_cast< QString >(interfaceWayland.property("Primary"));
                        } else {
                            screenname = QGuiApplication::primaryScreen()->name();
                        }
                        QDBusMessage reply = interface.call(QStringLiteral("SetMonitorBackground"), screenname, path);
                        qDebug() << "SettingWallpaper: replay" << reply.errorMessage();
                    } else {
                        qWarning() << "SettingWallpaper failed" << interface.lastError();
                    }
                }
                // Remove the tmp file
                QTimer::singleShot(5000, [ = ] {
                    QFile(path).remove();
                });


            }
        }
    });
    th1->start();
}

void LibViewPanel::setWallpaper(const QString &imgPath)
{
    QThread *th1 = QThread::create([ = ]() {
        if (!imgPath.isNull()) {
            QString path = imgPath;
            //202011/12 bug54279
            {
                //设置壁纸代码改变，采用DBus,原方法保留
                if (/*!qEnvironmentVariableIsEmpty("FLATPAK_APPID")*/1) {
                    // gdbus call -e -d com.deepin.daemon.Appearance -o /com/deepin/daemon/Appearance -m com.deepin.daemon.Appearance.Set background /home/test/test.png
                    qDebug() << "SettingWallpaper: " << "flatpak" << path;
                    QDBusInterface interface("com.deepin.daemon.Appearance",
                                                 "/com/deepin/daemon/Appearance",
                                                 "com.deepin.daemon.Appearance");
//                    if (interface.isValid()) {
//                        //获取鼠标在的位置的桌面
//                        QString screenname = QGuiApplication::screenAt(QCursor::pos())->name();
//                        QDBusMessage reply = interface.call(QStringLiteral("SetMonitorBackground"), screenname, path);
//                        qDebug() << "SettingWallpaper: replay" << reply.errorMessage();
//                    }
                    if (interface.isValid()) {
                        QString screenname;

                        //判断环境是否是wayland
                        auto e = QProcessEnvironment::systemEnvironment();
                        QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
                        QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

                        bool isWayland = false;
                        if (XDG_SESSION_TYPE != QLatin1String("wayland") && !WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
                            isWayland = false;
                        } else {
                            isWayland = true;
                        }
                        //wayland下设置壁纸使用，2020/09/21
                        if (isWayland) {
                            QDBusInterface interfaceWayland("com.deepin.daemon.Display", "/com/deepin/daemon/Display", "com.deepin.daemon.Display");
                            screenname = qvariant_cast< QString >(interfaceWayland.property("Primary"));
                        } else {
                            screenname = QGuiApplication::primaryScreen()->name();
                        }
                        QDBusMessage reply = interface.call(QStringLiteral("SetMonitorBackground"), screenname, path);
                        qDebug() << "SettingWallpaper: replay" << reply.errorMessage();
                    } else {
                        qWarning() << "SettingWallpaper failed" << interface.lastError();
                    }
                }
            }
        }
    });
    th1->start();
}

bool LibViewPanel::startdragImage(const QStringList &paths, const QString &firstPath)
{
    bool bRet = false;
    QStringList image_list = paths;
    if (image_list.isEmpty())
        return false;

    if (LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeLocal) {
        QString path = image_list.first();
        if ((path.indexOf("smb-share:server=") != -1 || path.indexOf("mtp:host=") != -1 || path.indexOf("gphoto2:host=") != -1)) {
            image_list.clear();
            //判断是否图片格式
            if (ImageEngine::instance()->isImage(path)) {
                image_list << path;
            }
        } else {
            QString DirPath = image_list.first().left(image_list.first().lastIndexOf("/"));
            QDir _dirinit(DirPath);
            QFileInfoList m_AllPath = _dirinit.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
            //修复Ｑt带后缀排序错误的问题
            std::sort(m_AllPath.begin(), m_AllPath.end(), compareByFileInfo);

            image_list.clear();
            for (int i = 0; i < m_AllPath.size(); i++) {
                QString tmpPath = m_AllPath.at(i).filePath();
                if (tmpPath.isEmpty()) {
                    continue;
                }
                //判断是否图片格式
                if (ImageEngine::instance()->isImage(tmpPath)) {
                    image_list << tmpPath;
                }
            }
        }
        if (image_list.count() > 0) {
            bRet = true;
        } else {
            bRet = false;
        }
        //解决拖入非图片文件会出现崩溃
        QString loadingPath = "";
        if (image_list.contains(path)) {
            loadingPath = path;
        } else if (image_list.count() > 0) {
            loadingPath = image_list.first();
        }
        //展示当前图片
        loadImage(loadingPath, image_list);

        QTimer::singleShot(500, [ = ] {
            //看图制作全部缩略图
            ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), image_list, image_list.size());
        });

    } else if (LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeAlbum) {
        //展示当前图片
        loadImage(firstPath, paths);
        ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), image_list, image_list.size());
    }
    m_bottomToolbar->thumbnailMoveCenterWidget();
    return bRet;
}

void LibViewPanel::setTopBarVisible(bool visible)
{
    if (m_topToolbar) {
        m_topToolBarIsAlwaysHide = !visible;
        m_topToolbar->setVisible(visible);
    }
}

void LibViewPanel::setBottomtoolbarVisible(bool visible)
{
    if (m_bottomToolbar) {
        m_isBottomBarVisble = visible;
        m_bottomToolbar->setVisible(visible);
    }
}

DIconButton *LibViewPanel::getBottomtoolbarButton(imageViewerSpace::ButtonType type)
{
    DIconButton *button = nullptr;
    if (m_bottomToolbar) {
        button = m_bottomToolbar->getBottomtoolbarButton(type);
    }
    return button;
}

QString LibViewPanel::getCurrentPath()
{
    return m_currentPath;
}

void LibViewPanel::setCurrentWidget(const QString &path)
{
    //存在切换到幻灯片被切换回去的情况,所以如果是当前界面为幻灯片,则不切换为其他的页面
    if (m_stack->currentWidget() != m_sliderPanel) {
        QFileInfo info(path);
        imageViewerSpace::ItemInfo ItemInfo = m_bottomToolbar->getCurrentItemInfo();
        //判断是否是损坏图片
        if (!info.isFile() && !path.isEmpty()) {
            if (m_thumbnailWidget) {
                m_stack->setCurrentWidget(m_thumbnailWidget);
                //损坏图片不透明
                emit m_view->sigImageOutTitleBar(false);
                m_thumbnailWidget->setThumbnailImageAndText(QPixmap::fromImage(ItemInfo.image), ThumbnailWidget::DamageType);
                if ((m_bottomToolbar->getAllFileCount() <= 1 && ImgViewerType::ImgViewerTypeAlbum != LibCommonService::instance()->getImgViewerType()) ||
                        (m_bottomToolbar->getAllFileCount() == 0 && ImgViewerType::ImgViewerTypeAlbum == LibCommonService::instance()->getImgViewerType())) {
                    emit ImageEngine::instance()->sigPicCountIsNull();
                }
            }
            if (m_nav) {
                m_nav->setVisible(false);
            }
        } else if (!info.permission(QFile::ReadUser)) {
            //额外判断是否是因为没有读权限导致裂图
            if (!info.permission(QFile::ReadUser)) {
                if (m_thumbnailWidget) {
                    m_stack->setCurrentWidget(m_thumbnailWidget);
                    //损坏图片不透明
                    emit m_view->sigImageOutTitleBar(false);
                    m_thumbnailWidget->setThumbnailImageAndText(QPixmap(), ThumbnailWidget::CannotReadType);
                    if (m_bottomToolbar->getAllFileCount() == 0) {
                        emit ImageEngine::instance()->sigPicCountIsNull();
                    }
                }
            }
            if (m_nav) {
                m_nav->setVisible(false);
            }
        } else if (!m_view->image().isNull()) {
            if (m_view) {
                m_stack->setCurrentWidget(m_view);
                //判断下是否透明
                m_view->titleBarControl();
            }
            //判断是否存在缓存
        } else if (ItemInfo.imageType == imageViewerSpace::ImageType::ImageTypeDamaged) {
            if (m_lockWidget) {
                m_stack->setCurrentWidget(m_lockWidget);
                //损坏图片不透明
                emit m_view->sigImageOutTitleBar(false);
            }
            if (m_nav) {
                m_nav->setVisible(false);
            }
        }
    }
}

void LibViewPanel::slotsDirectoryChanged(const QString &path)
{
    Q_UNUSED(path);
    if (m_view) {
        if (QFileInfo(m_currentPath).isReadable() && m_stack->currentWidget() != m_view) {
            m_view->onIsChangedTimerTimeout();
        }
        updateMenuContent();
    }
}

void LibViewPanel::setBottomToolBarButtonAlawysNotVisible(imageViewerSpace::ButtonType id, bool notVisible)
{
    if (m_bottomToolbar) {
        m_bottomToolbar->setButtonAlawysNotVisible(id, notVisible);
    }
}

bool LibViewPanel::startChooseFileDialog()
{
    bool bRet = false;
    if (m_stack->currentWidget() != m_sliderPanel) {
        QString filter = tr("All images");

        filter.append('(');
        filter.append(Libutils::image::supportedImageFormats().join(" "));
        filter.append(')');

        static QString cfgGroupName = QStringLiteral("General"),
                       cfgLastOpenPath = QStringLiteral("LastOpenPath");
        QString pictureFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        QDir existChecker(pictureFolder);
        if (!existChecker.exists()) {
            pictureFolder = QDir::currentPath();
        }

        pictureFolder = LibConfigSetter::instance()->value(cfgGroupName, cfgLastOpenPath, pictureFolder).toString();
#ifndef USE_TEST
        QStringList image_list =
            DFileDialog::getOpenFileNames(this, tr("Open Image"), pictureFolder, filter, nullptr,
                                          DFileDialog::HideNameFilterDetails);
#else
        QStringList image_list = QStringList(QApplication::applicationDirPath() + "/tif.tif");
#endif
        if (image_list.isEmpty())
            return false;

        QString path = image_list.first();
        QFileInfo firstFileInfo(path);
        LibConfigSetter::instance()->setValue(cfgGroupName, cfgLastOpenPath, firstFileInfo.path());

        if ((path.indexOf("smb-share:server=") != -1 || path.indexOf("mtp:host=") != -1 || path.indexOf("gphoto2:host=") != -1)) {
            image_list.clear();
            //判断是否图片格式
            if (ImageEngine::instance()->isImage(path)) {
                image_list << path;
            }
        } else {
            QString DirPath = image_list.first().left(image_list.first().lastIndexOf("/"));
            QDir _dirinit(DirPath);
            QFileInfoList m_AllPath = _dirinit.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
            //修复Ｑt带后缀排序错误的问题
            std::sort(m_AllPath.begin(), m_AllPath.end(), compareByFileInfo);

            image_list.clear();
            for (int i = 0; i < m_AllPath.size(); i++) {
                QString tmpPath = m_AllPath.at(i).filePath();
                if (tmpPath.isEmpty()) {
                    continue;
                }
                //判断是否图片格式
                if (ImageEngine::instance()->isImage(tmpPath)) {
                    image_list << tmpPath;
                }
            }
        }
        if (image_list.count() > 0) {
            bRet = true;
        } else {
            bRet = false;
        }
        QString loadingPath;
        if (image_list.contains(path)) {
            loadingPath = path;
        } else {
            loadingPath = image_list.first();
        }
        //展示当前图片
        loadImage(loadingPath, image_list);
        //启动线程制作缩略图
        if (LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeLocal ||
                LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeNull) {
            //看图首先制作显示的图片的缩略图
            ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), QStringList(path), 1);
            //看图制作全部缩略图
            ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), image_list, image_list.size());
        }
    }
    //ctrl+o打开后需要居中
    m_bottomToolbar->thumbnailMoveCenterWidget();
    return bRet;
}

void LibViewPanel::slotBottomMove()
{
    if (m_bottomToolbar) {
        if (window()->isFullScreen()) {
            QPoint pos = mapFromGlobal(QCursor::pos());
            //使用屏幕大小进行计算
            int number = QApplication::desktop()->screenNumber(this);
            int nParentWidth = QGuiApplication::screens().at(number)->geometry().width();
            int nParentHeight = QGuiApplication::screens().at(number)->geometry().height();
            if (nParentHeight - 20 < pos.y() && nParentHeight > pos.y() && nParentHeight == m_bottomToolbar->y()) {
                m_bottomAnimation = new QPropertyAnimation(m_bottomToolbar, "pos");
                m_bottomAnimation->setDuration(200);
                m_bottomAnimation->setEasingCurve(QEasingCurve::NCurveTypes);
                m_bottomAnimation->setStartValue(
                    QPoint((nParentWidth - m_bottomToolbar->width()) / 2, m_bottomToolbar->y()));
                m_bottomAnimation->setEndValue(QPoint((nParentWidth - m_bottomToolbar->width()) / 2,
                                                      nParentHeight - m_bottomToolbar->height() - 10));
                connect(m_bottomAnimation, &QPropertyAnimation::finished, this, [ = ]() {
                    delete m_bottomAnimation;
                    m_bottomAnimation = nullptr;
                });
                m_bottomAnimation->start();
            } else if (nParentHeight - m_bottomToolbar->height() - 10 > pos.y() &&
                       nParentHeight - m_bottomToolbar->height() - 10 == m_bottomToolbar->y()) {
                m_bottomAnimation = new QPropertyAnimation(m_bottomToolbar, "pos");
                m_bottomAnimation->setDuration(200);
                m_bottomAnimation->setEasingCurve(QEasingCurve::NCurveTypes);
                m_bottomAnimation->setStartValue(
                    QPoint((nParentWidth - m_bottomToolbar->width()) / 2, m_bottomToolbar->y()));
                m_bottomAnimation->setEndValue(QPoint((nParentWidth - m_bottomToolbar->width()) / 2, nParentHeight));
                connect(m_bottomAnimation, &QPropertyAnimation::finished, this, [ = ]() {
                    delete m_bottomAnimation;
                    m_bottomAnimation = nullptr;
                });
                m_bottomAnimation->start();
            } else if (m_bottomToolbar->y() < nParentHeight - 100) {
                //如果相差太远，则自动隐藏位置
                if (!m_bottomAnimation) {
                    m_bottomToolbar->move(m_bottomToolbar->x(), nParentHeight);
                }
            }
        } else {
            //如果非全屏，则显示m_bottomToolbar
            if (m_isBottomBarVisble) {
                m_bottomToolbar->setVisible(true);
            }

        }
    }
}

void LibViewPanel::showNext()
{
    DIconButton *NextButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypeNext);
    if (NextButton->isEnabled()) {
        m_bottomToolbar->onNextButton();
    }
}

void LibViewPanel::showPrevious()
{
    DIconButton *PreviousButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypePre);
    if (PreviousButton->isEnabled()) {
        m_bottomToolbar->onPreButton();
    }
}

void LibViewPanel::updateCustomAlbum(const QMap<QString, bool> &map, bool isFav)
{
    m_CustomAlbumName.clear();
    m_CustomAlbumName = map;
    m_isFav = isFav;
}

void LibViewPanel::setIsCustomAlbum(bool isCustom, const QString &album)
{
    m_isCustomAlbum = isCustom;
    m_CurrentCustomName = album;
}

bool LibViewPanel::slotOcrPicture()
{
    if (!m_ocrInterface) {
        initOcr();
    }
    QString path = m_bottomToolbar->getCurrentItemInfo().path;
    //图片过大，会导致崩溃，超过4K，智能裁剪
    if (m_ocrInterface != nullptr && m_view != nullptr) {
        QImage image = m_view->image();
        if (image.width() > 2500) {
            image = image.scaledToWidth(2500, Qt::SmoothTransformation);
        }
        if (image.height() > 2500) {
            image = image.scaledToHeight(2500, Qt::SmoothTransformation);
        }
        //替换为了保存为文件,用路径去打开ocr
        QFileInfo info(path);
        qDebug() << info.completeBaseName();
        QString savePath = IMAGE_TMPPATH + info.completeBaseName() + ".png";
        image.save(savePath);
        //采用路径，以防止名字出错
        m_ocrInterface->openFile(savePath);
    }
    return false;
}

void LibViewPanel::backImageView(const QString &path)
{
    m_stack->setCurrentWidget(m_view);
    if (path != "") {
//        m_view->setImage(path);
        m_bottomToolbar->setCurrentPath(path);
    }
    //每次退出的时候需要删除幻灯片,释放空间
    if (m_sliderPanel) {
        m_sliderPanel->deleteLater();
        m_sliderPanel = nullptr;
    }
    //退出幻灯片的时候导航栏应该出现(未打开不出现)
    if (m_nav && m_view) {
        m_nav->setVisible((!m_nav->isAlwaysHidden() && !m_view->isWholeImageVisible()) && !m_view->image().isNull());
    }
    //退出幻灯片，应该切换回应该的窗口
    //判断文件是否存在
    setCurrentWidget(path);

}

void LibViewPanel::initSlidePanel()
{
    if (!m_sliderPanel) {
        m_sliderPanel = new LibSlideShowPanel(this);
        m_stack->addWidget(m_sliderPanel);
        connect(m_sliderPanel, &LibSlideShowPanel::hideSlidePanel, this, &LibViewPanel::backImageView);
        connect(m_sliderPanel, &LibSlideShowPanel::hideSlidePanel, ImageEngine::instance(), &ImageEngine::exitSlideShow);
    }
}

void LibViewPanel::initLockPanel()
{
    if (!m_lockWidget) {
        m_lockWidget = new LockWidget("", "", this);
        m_stack->addWidget(m_lockWidget);
        connect(m_lockWidget, &LockWidget::sigMouseMove, this, &LibViewPanel::slotBottomMove);
        connect(m_lockWidget, &LockWidget::showfullScreen, this, &LibViewPanel::toggleFullScreen);

        //上一页，下一页信号连接
        connect(m_lockWidget, &LockWidget::previousRequested, this, &LibViewPanel::showPrevious);
        connect(m_lockWidget, &LockWidget::nextRequested, this, &LibViewPanel::showNext);
    }
}

void LibViewPanel::initThumbnailWidget()
{
    if (!m_thumbnailWidget) {
        m_thumbnailWidget = new ThumbnailWidget("", "", this);
        m_stack->addWidget(m_thumbnailWidget);
        connect(m_thumbnailWidget, &ThumbnailWidget::sigMouseMove, this, &LibViewPanel::slotBottomMove);
        connect(m_thumbnailWidget, &ThumbnailWidget::showfullScreen, this, &LibViewPanel::toggleFullScreen);

        //上一页，下一页信号连接
        connect(m_thumbnailWidget, &ThumbnailWidget::previousRequested, this, &LibViewPanel::showPrevious);
        connect(m_thumbnailWidget, &ThumbnailWidget::nextRequested, this, &LibViewPanel::showNext);
    }
}

void LibViewPanel::initShortcut()
{
    QShortcut *sc = nullptr;
    // Delay image toggle

    // Previous
    sc = new QShortcut(QKeySequence(Qt::Key_Left), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [ = ] {
        if (m_stack->currentWidget() != m_sliderPanel)
        {
            DIconButton *PreButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypePre);
            if (PreButton->isEnabled()) {
                m_bottomToolbar->onPreButton();
            }
        }
    });
    // Next
    sc = new QShortcut(QKeySequence(Qt::Key_Right), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [ = ] {
        if (m_stack->currentWidget() != m_sliderPanel)
        {
            DIconButton *NextButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypeNext);
            if (NextButton->isEnabled()) {
                m_bottomToolbar->onNextButton();
            }
        }
    });

    // Zoom out (Ctrl++ Not working, This is a confirmed bug in Qt 5.5.0)
    sc = new QShortcut(QKeySequence(Qt::Key_Up), this);
    sc->setContext(Qt::WindowShortcut);
    //fix 36530 当图片读取失败时（格式不支持、文件损坏、没有权限），不能进行缩放操作
    connect(sc, &QShortcut::activated, this, [ = ] {
        qDebug() << "Qt::Key_Up:";
        if (m_stack->currentWidget() != m_sliderPanel && !m_view->image().isNull())
        {
            m_view->setScaleValue(1.1);
        }
    });
    sc = new QShortcut(QKeySequence("Ctrl++"), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [ = ] {
        if (m_stack->currentWidget() != m_sliderPanel && QFile(m_view->path()).exists() && !m_view->image().isNull())
        {
            m_view->setScaleValue(1.1);
        }
    });
    sc = new QShortcut(QKeySequence("Ctrl+="), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [ = ] {
        if (m_stack->currentWidget() != m_sliderPanel && QFile(m_view->path()).exists() && !m_view->image().isNull())
        {
            m_view->setScaleValue(1.1);
        }
    });
    // Zoom in
    sc = new QShortcut(QKeySequence(Qt::Key_Down), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [ = ] {
        qDebug() << "Qt::Key_Down:";
        if (m_stack->currentWidget() != m_sliderPanel && QFile(m_view->path()).exists() && !m_view->image().isNull())
            m_view->setScaleValue(0.9);
    });
    sc = new QShortcut(QKeySequence("Ctrl+-"), this);
    sc->setContext(Qt::WindowShortcut);
    connect(sc, &QShortcut::activated, this, [ = ] {
        if (m_stack->currentWidget() != m_sliderPanel && QFile(m_view->path()).exists() && !m_view->image().isNull())
        {
            m_view->setScaleValue(0.9);
        }
    });
    // Esc
    QShortcut *esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    esc->setContext(Qt::WindowShortcut);
    connect(esc, &QShortcut::activated, this, [ = ] {
        if (m_stack->currentWidget() == m_sliderPanel)
        {
            m_sliderPanel->backToLastPanel();
            emit ImageEngine::instance()->escShortcutActivated(true);
        } else if (window()->isFullScreen())
        {
            if (m_stack->currentWidget() != m_thumbnailWidget && m_stack->currentWidget() != m_lockWidget) {
                m_stack->setCurrentWidget(m_view);
            }
//            toggleFullScreen();
            showNormal();
            m_view->viewport()->setCursor(Qt::ArrowCursor);
            //修复连续点击F5和esc的问题
            if (m_sliderPanel) {
                m_sliderPanel->onShowPause();
            }
            emit ImageEngine::instance()->escShortcutActivated(true);
        } else
        {
            //非全屏状态按ESC，等于按下m_back
            emit ImageEngine::instance()->escShortcutActivated(false);
        }
    });
    // 1:1 size
    QShortcut *adaptImage = new QShortcut(QKeySequence("Ctrl+0"), this);
    adaptImage->setContext(Qt::WindowShortcut);
    connect(adaptImage, &QShortcut::activated, this, [ = ] {
        if (m_stack->currentWidget() != m_sliderPanel)
        {
            if (QFile(m_view->path()).exists())
                m_view->fitImage();
        }
    });

}

void LibViewPanel::onMenuItemClicked(QAction *action)
{
    //当幻灯片的情况屏蔽快捷键的使用
    if (m_stack->currentWidget() != m_sliderPanel) {
        QString currentpath = m_bottomToolbar->getCurrentItemInfo().path;
        if (currentpath.isEmpty()) {
            currentpath = m_currentPath;
        }
        const int id = action->property("MenuID").toInt();
        switch (imageViewerSpace::NormalMenuItemId(id)) {
        case IdFullScreen:
        case IdExitFullScreen: {
            toggleFullScreen();
            break;
        }
        case IdStartSlideShow: {
            ViewInfo vinfo;
            vinfo.fullScreen = window()->isFullScreen();
            vinfo.lastPanel = this;
            vinfo.path = m_bottomToolbar->getCurrentItemInfo().path;
            vinfo.paths = m_bottomToolbar->getAllPath();
            vinfo.viewMainWindowID = 0;
            startSlideShow(vinfo);
            break;
        }
        case IdPrint: {
            if (m_view) {
                m_view->slotRotatePixCurrent();
            }
            //打开重命名窗口时关闭定时器
            killTimer(m_hideCursorTid);
            m_hideCursorTid = 0;
            m_view->viewport()->setCursor(Qt::ArrowCursor);

            PrintHelper::getIntance()->showPrintDialog(QStringList(m_bottomToolbar->getCurrentItemInfo().path), this);

            //开启定时器
            m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);
            break;
        }
        case IdRename: {
            if (m_view) {
                m_view->slotRotatePixCurrent();
            }
            //todo,重命名
            QString oldPath = m_bottomToolbar->getCurrentItemInfo().path;
            RenameDialog *renamedlg =  new RenameDialog(oldPath, this);

            QRect rect = this->geometry();
            QPoint globalPos = this->mapToGlobal(QPoint(0, 0));
            renamedlg->move(globalPos.x() + rect.width() / 2 - renamedlg->width() / 2, globalPos.y() + rect.height() / 2 - renamedlg->height() / 2);

            //打开重命名窗口时关闭定时器
            killTimer(m_hideCursorTid);
            m_hideCursorTid = 0;
            m_view->viewport()->setCursor(Qt::ArrowCursor);

#ifndef USE_TEST
            if (renamedlg->exec()) {
#else
            renamedlg->m_lineedt->setText("40_1");
            renamedlg->show();
            {
#endif
                QFile file(oldPath);
                QString filepath = renamedlg->GetFilePath();
                QString filename = renamedlg->GetFileName();
                bool bOk = file.rename(filepath);
                if (bOk) {
                    //to文件改变后做的事情
                    if (m_topToolbar) {
                        m_topToolbar->setMiddleContent(filename);
                        LibCommonService::instance()->reName(oldPath, filepath);
                        //重新打开该图片
                        m_bottomToolbar->setCurrentPath(filepath);
                        openImg(0, filepath);
                    }
                }
            }
            //开启定时器
            m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);
            break;
        }
        case IdCopy: {
            if (m_view) {
                m_view->slotRotatePixCurrent();
            }
            //todo,复制
            Libutils::base::copyImageToClipboard(QStringList(m_bottomToolbar->getCurrentItemInfo().path));
            break;
        }
        case IdMoveToTrash: {
            if (m_view) {
                m_view->slotRotatePixCurrent();
            }
            //todo,删除
            if (m_bottomToolbar) {
                m_bottomToolbar->onTrashBtnClicked();
            }
            break;
        }
        case IdShowNavigationWindow: {
            m_nav->setAlwaysHidden(false);
            break;
        }
        case IdHideNavigationWindow: {
            m_nav->setAlwaysHidden(true);
            break;
        }
        case IdRotateClockwise: {
            //todo旋转
            if (m_bottomToolbar) {
                m_bottomToolbar->onRotateRBtnClicked();
            }
            break;
        }
        case IdRotateCounterclockwise: {
            //todo旋转
            if (m_bottomToolbar) {
                m_bottomToolbar->onRotateLBtnClicked();
            }
            break;
        }
        case IdSetAsWallpaper: {
            if (m_view) {
                m_view->slotRotatePixCurrent();
            }
            //todo设置壁纸
            qDebug() << m_currentPath;
//            setWallpaper(m_view->image());
            if (!m_currentPath.isNull() && m_view->getcurrentImgCount() <= 1) {
                setWallpaper(m_currentPath);
            } else {
                setWallpaper(m_view->image());
            }
            break;
        }
        case IdDisplayInFileManager : {
            if (m_view) {
                m_view->slotRotatePixCurrent();
            }
            //todo显示在文管
            Libutils::base::showInFileManager(m_bottomToolbar->getCurrentItemInfo().path);
            break;
        }
        case IdImageInfo: {
            if (m_view) {
                m_view->slotRotatePixCurrent();
            }
            //todo,文件信息
            if (!m_info && !m_extensionPanel) {
                initExtensionPanel();
            }
            //重新刷新文件信息
            m_info->updateInfo();
            m_info->show();

            //判断是否有缓存,无缓存,则使用打开路径
            QString path = m_bottomToolbar->getCurrentItemInfo().path;
            if (path.isEmpty()) {
                path = m_currentPath;
            }
            m_info->setImagePath(path);
            m_extensionPanel->setContent(m_info);
            //清除焦点
            m_extensionPanel->setFocus(Qt::NoFocusReason);
            m_extensionPanel->show();
            if (this->window()->isFullScreen() || this->window()->isMaximized()) {
                m_extensionPanel->move(this->window()->width() - m_extensionPanel->width() - 24,
                                       TOP_TOOLBAR_HEIGHT * 2);
            } else {
                m_extensionPanel->move(this->window()->pos() +
                                       QPoint(this->window()->width() - m_extensionPanel->width() - 24,
                                              TOP_TOOLBAR_HEIGHT * 2));
            }
            break;
        }
        case IdOcr: {
            if (m_view) {
                m_view->slotRotatePixCurrent();
            }
            //todo,ocr
            slotOcrPicture();
            break;
        }
        case IdAddToAlbum: {
            const QString album = action->data().toString();
            if (album != "Add to new album") {
                emit ImageEngine::instance()->sigAddToAlbum(false, album, currentpath);
            } else {
                emit ImageEngine::instance()->sigAddToAlbum(true, "", currentpath);
            }
            break;
        }
        case IdExport: {
            emit ImageEngine::instance()->sigExport(currentpath);
            break;
        }
        case IdRemoveFromAlbum: {
            emit ImageEngine::instance()->sigRemoveFromCustom(currentpath, m_CurrentCustomName);
            //从相册中移除时，本库展示的也需要移除
            m_bottomToolbar->deleteImage();
            break;
        }
        case IdAddToFavorites: {
            emit ImageEngine::instance()->sigAddOrRemoveToFav(currentpath, true);
            emit ImageEngine::instance()->sigUpdateCollectBtn();
            break;
        }
        case IdRemoveFromFavorites: {
            emit ImageEngine::instance()->sigAddOrRemoveToFav(currentpath, false);
            emit ImageEngine::instance()->sigUpdateCollectBtn();
            break;
        }
        default:
            break;
        }
    }
}

void LibViewPanel::slotOneImgReady(QString path, imageViewerSpace::ItemInfo itemInfo)
{
    imageViewerSpace::ItemInfo ItemInfo = m_bottomToolbar->getCurrentItemInfo();
    if (path.contains(ItemInfo.path)) {
        updateMenuContent();
    }
    Q_UNUSED(itemInfo);
}

void LibViewPanel::startSlideShow(const ViewInfo &info)
{
    //判断旋转图片本体是否旋转
    if (m_view) {
        m_view->slotRotatePixCurrent();
    }
    //todo,幻灯片
    if (!m_sliderPanel) {
        initSlidePanel();
    }
    m_sliderPanel->startSlideShow(info);
    m_stack->setCurrentWidget(m_sliderPanel);
    //打开幻灯片需要隐藏工具栏
    slotBottomMove();
    //正在滑动缩略图的时候不再显示
    if (m_nav->isVisible()) {
        m_nav->setVisible(false);
    }
    //打开幻灯片默认关闭图片详情
    if (m_info && m_extensionPanel) {
        m_info->setVisible(false);
        m_extensionPanel->setVisible(false);
    }
}

void LibViewPanel::resetBottomToolbarGeometry(bool visible)
{
//    m_bosetVisiblele);
    if (m_isBottomBarVisble) {
        m_bottomToolbar->setVisible(visible);
    }
    if (visible) {
        int width = qMin(m_bottomToolbar->getToolbarWidth() + 22, (this->width() - RT_SPACING));
        int x = (this->width() - width) / 2;
        //窗口高度-工具栏高度-工具栏到底部距离
        //全屏默认隐藏
        int y = this->height();
        if (!window()->isFullScreen()) {
            y = this->height() - BOTTOM_TOOLBAR_HEIGHT - BOTTOM_SPACING;
        }
        m_bottomToolbar->setGeometry(x, y, width, BOTTOM_TOOLBAR_HEIGHT);
    }
}

void LibViewPanel::openImg(int index, QString path)
{
    //展示图片
    m_view->slotRotatePixCurrent();
    m_view->setImage(path);
    m_view->resetTransform();
    QFileInfo info(path);
    m_topToolbar->setMiddleContent(info.fileName());
    m_currentPath = path;
    //刷新收藏按钮
    emit ImageEngine::instance()->sigUpdateCollectBtn();
    updateMenuContent(path);
    Q_UNUSED(index);
}

void LibViewPanel::slotRotateImage(int angle)
{
    if (m_view) {
        if (m_view->loadPhase() == LibImageGraphicsView::ThumbnailFinish) {
            m_view->setNewImageRotateAngle(angle);
        }
        m_view->slotRotatePixmap(angle);
    }

    //实时保存太卡，因此采用2s后延时保存的问题
    if (!m_tSaveImage) {
        m_tSaveImage = new QTimer(this);
        connect(m_tSaveImage, &QTimer::timeout, this, [ = ]() {
            m_view->slotRotatePixCurrent();
        });
    }
    m_tSaveImage->setSingleShot(true);
    m_tSaveImage->start(2000);
}

void LibViewPanel::slotResetTransform(bool bRet)
{
    if (bRet && m_view) {
        m_view->fitWindow();
    } else if (!bRet && m_view) {
        m_view->fitImage();
    }
}


void LibViewPanel::resizeEvent(QResizeEvent *e)
{
    if (m_extensionPanel) {
        // 获取widget左上角坐标的全局坐标
        //lmh0826,解决bug44826
        QPoint p = this->mapToGlobal(QPoint(0, 0));
        m_extensionPanel->move(p + QPoint(this->window()->width() - m_extensionPanel->width() - 24,
                                          TOP_TOOLBAR_HEIGHT * 2));
    }
    if (this->m_topToolbar) {

        if (window()->isFullScreen()) {
            this->m_topToolbar->setVisible(false);

        } else {
            if (!m_topToolBarIsAlwaysHide) {
                this->m_topToolbar->setVisible(true);
            }
        }

        if (m_topToolbar->isVisible()) {
            this->m_topToolbar->resize(width(), 50);
        }
    }
    //当view处于适应窗口状态的时候,resize也会继承状态
    if (m_stack->currentWidget() == m_view) {
        if (m_view->isFitImage()) {
            m_view->fitImage();
        } else if (m_view->isFitWindow()) {
            m_view->fitWindow();
        }
    }

//    resetBottomToolbarGeometry(m_stack->currentWidget() == m_view);
    resetBottomToolbarGeometry(true);
    QFrame::resizeEvent(e);
    emit m_view->transformChanged();
}

void LibViewPanel::showEvent(QShowEvent *e)
{
    if (this->m_topToolbar) {
        m_topToolbar->resize(width(), 50);
    }
//    resetBottomToolbarGeometry(m_stack->currentWidget() == m_view);
    QFrame::showEvent(e);
}

void LibViewPanel::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    //    qDebug() << "windows flgs ========= " << this->windowFlags() << "attributs = " << this->testAttribute(Qt::WA_Resized);
}

void LibViewPanel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::ForwardButton) {
        DIconButton *preButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypePre);
        preButton->clicked();
    } else if (event->button() == Qt::BackButton) {
        DIconButton *nextButton = m_bottomToolbar->getBottomtoolbarButton(imageViewerSpace::ButtonTypeNext);
        nextButton->clicked();
    }
    QFrame::mousePressEvent(event);
}


void LibViewPanel::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (!pluginUtils::base::checkMimeData(mimeData)) {
        return;
    }
    event->setDropAction(Qt::CopyAction);
    event->accept();
    event->acceptProposedAction();
    DWidget::dragEnterEvent(event);
}

void LibViewPanel::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void LibViewPanel::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    QStringList paths;
    for (QUrl url : urls) {
        //lmh0901判断是否是图片
        QString path = url.toLocalFile();
        if (path.isEmpty()) {
            path = url.path();
        }
        paths << path;
    }
    startdragImage(paths);
}

void LibViewPanel::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_hideCursorTid && (!m_menu || !m_menu->isVisible())) {
        m_view->viewport()->setCursor(Qt::BlankCursor);
    }

    QFrame::timerEvent(e);
}
