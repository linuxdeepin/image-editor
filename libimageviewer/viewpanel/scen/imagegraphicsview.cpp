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
#include "imagegraphicsview.h"
#include "unionimage/pluginbaseutils.h"

#include <QDebug>
#include <QFile>
#include <QOpenGLWidget>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMovie>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QGraphicsPixmapItem>
#include <QPaintEvent>
#include <QtConcurrent>
#include <QHBoxLayout>
#include <qmath.h>
#include <QScrollBar>
#include <QGestureEvent>
#include <QSvgRenderer>
#include <QtGlobal>
#include <QDesktopWidget>
#include <QShortcut>
#include <QApplication>
#include <DSpinner>

#include "graphicsitem.h"
#include "imagesvgitem.h"
#include "unionimage/baseutils.h"
#include "unionimage/imageutils.h"
#include "unionimage/unionimage.h"
#include "accessibility/ac-desktop-define.h"
#include "../contents/morepicfloatwidget.h"
#include "imageengine.h"

#include <DGuiApplicationHelper>
#include <DApplicationHelper>


#ifndef QT_NO_OPENGL
//#include <QGLWidget>
#endif

#include <sys/inotify.h>
#include "service/commonservice.h"

DWIDGET_USE_NAMESPACE

namespace {

const QColor LIGHT_CHECKER_COLOR = QColor("#FFFFFF");
const QColor DARK_CHECKER_COLOR = QColor("#CCCCCC");
const QSize SPINNER_SIZE = QSize(40, 40);
#ifndef tablet_PC
const qreal MAX_SCALE_FACTOR = 20.0;
const qreal MIN_SCALE_FACTOR = 0.02;
#else
const qreal MAX_SCALE_FACTOR = 2.0;
qreal MIN_SCALE_FACTOR = 0.0;
#endif

QVariantList cachePixmap(const QString &path)
{
    QImage tImg;
    QString errMsg;
    QSize size;
    Q_UNUSED(size);
//    UnionImage_NameSpace::loadStaticImageFromFile(path, tImg, size, errMsg);
    LibUnionImage_NameSpace::loadStaticImageFromFile(path, tImg, errMsg);
    QPixmap p = QPixmap::fromImage(tImg);
    if (QFileInfo(path).exists() && p.isNull()) {
        //判定为损坏图片
//        p = utils::image::getDamagePixmap(DApplicationHelper::instance()->themeType() == DApplicationHelper::LightType);
        qDebug() << errMsg;
    }
    QVariantList vl;
    vl << QVariant(path) << QVariant(p);
    return vl;
}

}  // namespace
LibImageGraphicsView::LibImageGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
    , m_renderer(Native)
    , m_pool(new QThreadPool(this))
//    , m_svgItem(nullptr)
    , m_movieItem(nullptr)
    , m_pixmapItem(nullptr)
{
    this->setObjectName("ImageView");
//    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    setScene(new QGraphicsScene(this));
    setContentsMargins(0, 0, 0, 0);
    setMouseTracking(true);
    setTransformationAnchor(AnchorUnderMouse);
    setDragMode(ScrollHandDrag);
    setViewportUpdateMode(FullViewportUpdate);
    setAcceptDrops(false);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::Shape::NoFrame);

    viewport()->setCursor(Qt::ArrowCursor);

    //鼠标事件,以及不在通过键盘移动视图
    this->setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PanGesture);

    connect(&m_watcher, &QFutureWatcherBase::finished, this, &LibImageGraphicsView::onCacheFinish);
//    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this, &ImageView::onThemeChanged);
    m_pool->setMaxThreadCount(1);
    m_loadTimer = new QTimer(this);
    m_loadTimer->setSingleShot(true);
    m_loadTimer->setInterval(300);

    connect(m_loadTimer, &QTimer::timeout, this, &LibImageGraphicsView::onLoadTimerTimeout);
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &LibImageGraphicsView::onThemeTypeChanged);
    //初始化主题
    onThemeTypeChanged();
    m_imgFileWatcher = new QFileSystemWatcher(this);
    connect(m_imgFileWatcher, &QFileSystemWatcher::fileChanged, this, &LibImageGraphicsView::onImgFileChanged);
    m_isChangedTimer = new QTimer(this);
    QObject::connect(m_isChangedTimer, &QTimer::timeout, this, &LibImageGraphicsView::onIsChangedTimerTimeout);

    //让默认的快捷键失效，默认会滑动窗口
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), this), &QShortcut::activated, this, &LibImageGraphicsView::slotsUp);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), this), &QShortcut::activated, this, &LibImageGraphicsView::slotsDown);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right), this);

    new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Left), this);
    new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Right), this);
    new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Up), this);
    new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Down), this);

    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Left), this);
    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right), this);
    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Up), this);
    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Down), this);

    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down), this);

    new QShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Left), this);
    new QShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Right), this);
    new QShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Up), this);
    new QShortcut(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Down), this);

    new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Left), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Right), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Up), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Down), this);

    new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::SHIFT + Qt::Key_Left), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::SHIFT + Qt::Key_Right), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::SHIFT + Qt::Key_Up), this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::SHIFT + Qt::Key_Down), this);
}

int LibImageGraphicsView::getcurrentImgCount()
{
    int ret = 0;
    if (m_imageReader) {
        ret = m_imageReader->imageCount();
    }
    return ret;
}

LibImageGraphicsView::~LibImageGraphicsView()
{
    if (m_imgFileWatcher) {
//        m_imgFileWatcher->clear();
//        m_imgFileWatcher->quit();
//        m_imgFileWatcher->terminate();
//        m_imgFileWatcher->wait();
        m_imgFileWatcher->deleteLater();
    }
    if (m_imageReader) {
        delete m_imageReader;
        m_imageReader = nullptr;
    }
    if (m_morePicFloatWidget) {
        m_morePicFloatWidget->deleteLater();
        m_morePicFloatWidget = nullptr;
    }
    if (m_movieItem) {
        delete m_movieItem;
        m_movieItem = nullptr;
    }
    if (m_pixmapItem) {
        delete m_pixmapItem;
        m_pixmapItem = nullptr;
    }
    if (m_imgSvgItem) {
        delete m_imgSvgItem;
        m_imgSvgItem = nullptr;
    }
    //保存旋转状态
    slotRotatePixCurrent();
}

void LibImageGraphicsView::clear()
{
    if (m_pixmapItem != nullptr) {
        delete m_pixmapItem;
        m_pixmapItem = nullptr;
    }
    m_movieItem = nullptr;
    scene()->clear();
}

void LibImageGraphicsView::setImage(const QString &path, const QImage &image)
{
    if (m_spinner) {
        m_spinner->deleteLater();
        m_spinner = nullptr;
    }
    //默认多页图的按钮显示为false
    if (m_morePicFloatWidget) {
        m_morePicFloatWidget->setVisible(false);
        m_morePicFloatWidget->getButtonDown()->clearFocus();
    }
    if (m_imageReader) {
        delete m_imageReader;
        m_imageReader = nullptr;
    }
    //重新生成数据缓存
//    ImageEngine::instance()->makeImgThumbnail(CommonService::instance()->getImgSavePath(), QStringList(path), 1, true);
    //检测数据缓存,如果存在,则使用缓存
    imageViewerSpace::ItemInfo info = LibCommonService::instance()->getImgInfoByPath(path);
    m_bRoate = ImageEngine::instance()->isRotatable(path); //是否可旋转

    m_loadPath = path;
    // Empty path will cause crash in release-build mode
    if (path.isEmpty()) {
        return;
    }
    m_imgFileWatcher->removePath(m_path);
    m_path = path;
    m_imgFileWatcher->addPath(m_path);
    QString strfixL = QFileInfo(path).suffix().toUpper();
    QGraphicsScene *s = scene();
    QFileInfo fi(path);
    QStringList fList = LibUnionImage_NameSpace::supportMovieFormat(); //"gif","mng"
    //QMovie can't read frameCount of "mng" correctly,so change
    //the judge way to solve the problem

    imageViewerSpace::ImageType Type = info.imageType;
    if (Type == imageViewerSpace::ImageTypeBlank) {
        Type = LibUnionImage_NameSpace::getImageType(path);
    }
    //ImageTypeDynamic
    if (Type == imageViewerSpace::ImageTypeDynamic) {
        m_pixmapItem = nullptr;
        m_movieItem = nullptr;
        m_imgSvgItem = nullptr;
        s->clear();
        resetTransform();
        m_movieItem = new LibGraphicsMovieItem(path, strfixL);
        //        m_movieItem->start();
        // Make sure item show in center of view after reload
        setSceneRect(m_movieItem->boundingRect());
        qDebug() << "m_movieItem->boundingRect() = " << m_movieItem->boundingRect();
        s->addItem(m_movieItem);
        emit imageChanged(path);
        QMetaObject::invokeMethod(this, [ = ]() {
            resetTransform();
            autoFit();
        }, Qt::QueuedConnection);
        m_newImageLoadPhase = FullFinish;
    } else if (Type == imageViewerSpace::ImageTypeSvg) {
        m_pixmapItem = nullptr;
        m_movieItem = nullptr;
        m_imgSvgItem = nullptr;
        s->clear();
        resetTransform();

        if (!m_svgRenderer) {
            m_svgRenderer = new QSvgRenderer(this);
        }
        m_svgRenderer->load(path);
        m_imgSvgItem = new LibImageSvgItem();
        m_imgSvgItem->setSharedRenderer(m_svgRenderer);
        //不会出现锯齿
        m_imgSvgItem->setCacheMode(QGraphicsItem::NoCache);
        setSceneRect(m_imgSvgItem->boundingRect());
        s->addItem(m_imgSvgItem);
        emit imageChanged(path);
        QMetaObject::invokeMethod(this, [ = ]() {
            resetTransform();
            autoFit();
        }, Qt::QueuedConnection);
        m_newImageLoadPhase = FullFinish;
    } else {
        //当传入的image无效时，需要重新读取数据
        m_pixmapItem = nullptr;
        m_movieItem = nullptr;
        m_imgSvgItem = nullptr;
        scene()->clear();
        resetTransform();

        if (image.isNull()) {
            QPixmap pix ;
            if (!info.image.isNull()) {
                QImageReader imagreader(path);      //取原图的分辨率
                int w = imagreader.size().width();
                int h = imagreader.size().height();

                int wScale = 0;
                int hScale = 0;
                int wWindow = 0;
                int hWindow = 0;
                if (QApplication::activeWindow()) {
                    wWindow = QApplication::activeWindow()->width();
                    hWindow = QApplication::activeWindow()->height();
                } else {
                    wWindow = 1300;
                    hWindow = 848;
                }

                if (w >= wWindow) {
                    wScale = wWindow;
                    hScale = wScale * h / w;
                    if (hScale > hWindow) {
                        hScale = hWindow;
                        wScale = hScale * w / h;
                    }
                } else if (h >= hWindow) {
                    hScale = hWindow;
                    wScale = hScale * w / h;
                    if (wScale >= wWindow) {
                        wScale = wWindow;
                        hScale = wScale * h / w;
                    }
                } else {
                    wScale = w;
                    hScale = h;
                }
                if (wScale == 0 || wScale == -1) { //进入这个地方说明QImageReader未识别出图片
                    if (info.imgOriginalWidth > wWindow || info.imgOriginalHeight > hWindow) {
                        wScale = wWindow;
                        hScale = hWindow;
                    } else {
                        wScale = info.imgOriginalWidth;
                        hScale = info.imgOriginalHeight;
                    }
                }

                pix = QPixmap::fromImage(info.image).scaled(wScale, hScale, Qt::KeepAspectRatio);

                //存在缩放比问题需要setDevicePixelRatio
//                if (wScale < wWindow && hScale < hWindow) {
                pix.setDevicePixelRatio(devicePixelRatioF());
//                }
            }
            if (pix.isNull()) {
                //spinner
                if (!m_spinner) {
                    m_spinner = new DSpinner;
                    m_spinner->setFixedSize(SPINNER_SIZE);
                }
                m_spinner->start();

                // Make sure item show in center of view after reload
                setSceneRect(m_spinner->rect());
                s->addWidget(m_spinner);
            }
            m_pixmapItem = new LibGraphicsPixmapItem(pix);
            m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);

            // Make sure item show in center of view after reload
            if (!m_blurEffect) {
                m_blurEffect = new QGraphicsBlurEffect(this);
            }
            m_blurEffect->setBlurRadius(5);
            m_blurEffect->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
            m_pixmapItem->setGraphicsEffect(m_blurEffect);

            //如果缩略图不为空,则区域变为m_pixmapItem
            if (!pix.isNull()) {
                setSceneRect(m_pixmapItem->boundingRect());
            }
            //第一次打开直接启动,不使用延时300ms
            if (m_isFistOpen) {
                onLoadTimerTimeout();
                m_isFistOpen = false;
            } else {
                m_loadTimer->start();
            }
            scene()->addItem(m_pixmapItem);
            emit imageChanged(path);
            QMetaObject::invokeMethod(this, [ = ]() {
                resetTransform();
            }, Qt::QueuedConnection);
            m_newImageLoadPhase = ThumbnailFinish;
        } else {
            //当传入的image有效时，直接刷入图像，不再重复读取
            QPixmap pix = QPixmap::fromImage(image);
            pix.setDevicePixelRatio(devicePixelRatioF());
            m_pixmapItem = new LibGraphicsPixmapItem(pix);
            m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
            setSceneRect(m_pixmapItem->boundingRect());
            scene()->clear();
            scene()->addItem(m_pixmapItem);
            autoFit();
            emit imageChanged(path);
            this->update();
            emit hideNavigation();
            m_newImageLoadPhase = FullFinish;
        }
        if (Type == imageViewerSpace::ImageTypeMulti) {
            if (!m_morePicFloatWidget) {
                initMorePicWidget();
            }

            m_imageReader = new QImageReader(path);
            if (m_imageReader->imageCount() > 1) {
                //校验tiff的第一个图
//                m_imageReader->jumpToImage(0);
//                auto tifImg = m_imageReader->read();
//                if (tifImg.isNull()) {
//                    m_morePicFloatWidget->setVisible(false);
//                } else {
                //以免出现焦点在down的按钮下
                m_morePicFloatWidget->setFocus();
                m_morePicFloatWidget->setVisible(true);
                if (m_morePicFloatWidget->getButtonUp()) {
                    m_morePicFloatWidget->getButtonUp()->setEnabled(false);
                }
                if (m_morePicFloatWidget->getButtonDown()) {
                    m_morePicFloatWidget->getButtonDown()->setEnabled(true);
                }
                m_currentMoreImageNum = 0;
//                }
            } else {
                m_morePicFloatWidget->setVisible(false);
            }
            m_morePicFloatWidget->setLabelText(QString::number(m_imageReader->currentImageNumber() + 1) + "/" + QString::number(m_imageReader->imageCount()));
            m_morePicFloatWidget->move(this->width() - 80, this->height() / 2 - 50);
        }
    }
    m_firstset = true;
}

void LibImageGraphicsView::setScaleValue(qreal v)
{
    //预先计算需要的缩放比
    double temp = m_scal * v;
    double scaleFactor = -1.0;
    if (v < 1 && temp <= MIN_SCALE_FACTOR) {
        scaleFactor = MIN_SCALE_FACTOR / m_scal;
    } else if (v > 1 && temp >= MAX_SCALE_FACTOR) {
        scaleFactor = MAX_SCALE_FACTOR / m_scal;
    } else {
        scaleFactor = v;
        m_isFitImage = false;
        m_isFitWindow = false;
    }

    //执行缩放
    m_scal *= scaleFactor;
    scale(scaleFactor, scaleFactor);
    qDebug() << m_scal;

    //1:1高亮按钮
    if (m_scal - 1 > -0.01 && m_scal - 1 < 0.01) {
        emit checkAdaptImageBtn();
    } else {
        emit disCheckAdaptImageBtn();
    }

    //缩放通知信号
    emit scaled(m_scal * 100);
    emit showScaleLabel();
    emit transformChanged();
    titleBarControl();
}

void LibImageGraphicsView::autoFit()
{
    //确认场景加载出来后，才能调用场景内的item
//    if (!scene()->isActive())
//        return;
    if (image().isNull())
        return;
    QSize image_size = image().size();
    if ((image_size.width() >= width() ||
            image_size.height() >= height()) &&
            width() > 0 && height() > 0) {
        fitWindow();
    } else {
        fitImage();
    }
}

const QImage LibImageGraphicsView::image()
{
    QImage img;
    if (m_movieItem) {           // bit-map
        img = m_movieItem->pixmap().toImage();
    } else if (m_pixmapItem) {
        //FIXME: access to m_pixmapItem will crash
        if (nullptr == m_pixmapItem) {  //add to slove crash by shui
            img = QImage();
        } else {
            img = m_pixmapItem->pixmap().toImage();
        }

    } else if (m_imgSvgItem) { // 新增svg的image
        QImage image(m_imgSvgItem->renderer()->defaultSize(), QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor(0, 0, 0, 0));
        QPainter imagePainter(&image);
        m_imgSvgItem->renderer()->render(&imagePainter);
        imagePainter.end();
        img = image;
    } else {
        img = QImage();
    }
    if (img.isNull() && m_morePicFloatWidget) {
        m_morePicFloatWidget->setVisible(false);
    } else if (m_imageReader && (m_imageReader->imageCount() > 1) && m_morePicFloatWidget) {
        m_morePicFloatWidget->setVisible(true);
    }
    return img;
}

void LibImageGraphicsView::fitWindow()
{
    qreal wrs = windowRelativeScale();
    resetTransform();
    //可能存在比例超过20.0的情况，设置为20.0
    if (wrs > 20.0) {
        wrs = 20.0;
    }
    m_scal = wrs; //注意，这个东西的初始化要和附近的scale函数同步
    scale(wrs, wrs);
    emit checkAdaptScreenBtn();
    if (wrs - 1 > -0.01 &&
            wrs - 1 < 0.01) {
        emit checkAdaptImageBtn();
    } else {
        emit disCheckAdaptImageBtn();
    }
    m_isFitImage = false;
    m_isFitWindow = true;
    scaled(imageRelativeScale() * 100);
    emit transformChanged();

    titleBarControl();
}

void LibImageGraphicsView::fitImage()
{
    qreal wrs = windowRelativeScale();
    resetTransform();
    scale(1, 1);
    m_scal = 1; //注意，这个东西的初始化要和附近的scale函数同步
    emit checkAdaptImageBtn();
    if (wrs - 1 > -0.01 &&
            wrs - 1 < 0.01) {
        emit checkAdaptScreenBtn();
    } else {
        emit disCheckAdaptScreenBtn();
    }
    m_isFitImage = true;
    m_isFitWindow = false;
    scaled(imageRelativeScale() * 100);
    emit transformChanged();

    titleBarControl();
}

void LibImageGraphicsView::rotateClockWise()
{
    QString errMsg;
    QImage rotateResult;
    if (!LibUnionImage_NameSpace::rotateImageFIleWithImage(90, rotateResult, m_path, errMsg)) {
        qDebug() << errMsg;
        return;
    }
//    dApp->m_imageloader->updateImageLoader(QStringList(m_path), QList<QImage>({rotateResult}));
    setImage(m_path, rotateResult);
}

void LibImageGraphicsView::rotateCounterclockwise()
{
    QString errMsg;
    QImage rotateResult;
    if (!LibUnionImage_NameSpace::rotateImageFIleWithImage(-90, rotateResult, m_path, errMsg)) {
        qDebug() << errMsg;
        return;
    }
//    dApp->m_imageloader->updateImageLoader(QStringList(m_path), QList<QImage>({rotateResult}));
    setImage(m_path, rotateResult);
}

void LibImageGraphicsView::centerOn(qreal x, qreal y)
{
    QGraphicsView::centerOn(x, y);
    emit transformChanged();
}


qreal LibImageGraphicsView::imageRelativeScale() const
{
    // vertical scale factor are equal to the horizontal one
    return transform().m11();
}

qreal LibImageGraphicsView::windowRelativeScale() const
{
    QRectF bf = sceneRect();
    if (1.0 * width() / height() > 1.0 * bf.width() / bf.height()) {
        return 1.0 * height() / bf.height();
    } else {
        return 1.0 * width() / bf.width();
    }
}

const QString LibImageGraphicsView::path() const
{
    return m_path;
}

QPoint LibImageGraphicsView::mapToImage(const QPoint &p) const
{
    return viewportTransform().inverted().map(p);
}

QRect LibImageGraphicsView::mapToImage(const QRect &r) const
{
    return viewportTransform().inverted().mapRect(r);
}

QRect LibImageGraphicsView::visibleImageRect() const
{
    return mapToImage(rect()) & QRect(0, 0, static_cast<int>(sceneRect().width()), static_cast<int>(sceneRect().height()));
}

bool LibImageGraphicsView::isWholeImageVisible() const
{
    const QRect &r = visibleImageRect();
    const QRectF &sr = sceneRect();

    //社区版qt5.15.1存在偏差0-1
    return r.width() >= (sr.width() - 1) && r.height() >= (sr.height() - 1);
}

bool LibImageGraphicsView::isFitImage() const
{
    return m_isFitImage;
}

bool LibImageGraphicsView::isFitWindow() const
{
    return m_isFitWindow;
}

void LibImageGraphicsView::initMorePicWidget()
{
    m_morePicFloatWidget = new MorePicFloatWidget(this);
    m_morePicFloatWidget->initUI();

    connect(m_morePicFloatWidget->getButtonUp(), &DIconButton::clicked, this, &LibImageGraphicsView::slotsUp);
    connect(m_morePicFloatWidget->getButtonDown(), &DIconButton::clicked, this, &LibImageGraphicsView::slotsDown);

    m_morePicFloatWidget->setFixedWidth(70);
    m_morePicFloatWidget->setFixedHeight(140);
    m_morePicFloatWidget->show();
}

void LibImageGraphicsView::titleBarControl()
{
    qreal realHeight = 0.0;
    //简化image()的使用
    QImage img = image();
    if (m_movieItem /*|| m_imgSvgItem*/) {
        realHeight = img.size().height() * imageRelativeScale() * devicePixelRatioF();

    } else {
        realHeight = img.size().height() * imageRelativeScale();
    }

    if (realHeight > height() - 100) {
        emit sigImageOutTitleBar(true);
    } else {
        emit sigImageOutTitleBar(false);
    }
}

void LibImageGraphicsView::slotSavePic()
{
    //保存旋转的图片
    slotRotatePixCurrent();
}

void LibImageGraphicsView::onImgFileChanged(const QString &ddfFile)
{
    Q_UNUSED(ddfFile)
    m_isChangedTimer->start(200);
}

void LibImageGraphicsView::onLoadTimerTimeout()
{
    QFuture<QVariantList> f = QtConcurrent::run(m_pool, cachePixmap, m_loadPath);
    if (m_watcher.isRunning()) {
        m_watcher.cancel();
        m_watcher.waitForFinished();
    }
    m_watcher.setFuture(f);
    emit hideNavigation();

    //重新刷新缓存
    //ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), QStringList(m_loadPath), 1, true);
}

void LibImageGraphicsView::onThemeTypeChanged()
{
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::DarkType) {
        m_backgroundColor = Libutils::common::DARK_BACKGROUND_COLOR;
    } else {
        m_backgroundColor = Libutils::common::LIGHT_BACKGROUND_COLOR;
    }
    update();
}

void LibImageGraphicsView::onIsChangedTimerTimeout()
{
    QFileInfo file(m_path);
    if (file.exists()) {
//        dApp->m_imageloader->updateImageLoader(QStringList(m_path));
        setImage(m_path);
    }
    //刷新文件信息
    emit sigFIleDelete();
    m_isChangedTimer->stop();

}

void LibImageGraphicsView::slotsUp()
{
    if (m_morePicFloatWidget) {
        if (m_morePicFloatWidget->getButtonUp()) {
            m_morePicFloatWidget->getButtonUp()->setEnabled(true);
        }
        if (m_morePicFloatWidget->getButtonDown()) {
            m_morePicFloatWidget->getButtonDown()->setEnabled(true);
        }

        if (m_pixmapItem && m_imageReader && m_imageReader->imageCount() > 1) {
            if ((0 == m_imageReader->currentImageNumber()) && (0 == m_currentMoreImageNum)) {
                //改为与现在相同按钮点击逻辑相同修改bug62227，第一张图片时候，向上按钮置灰
                m_morePicFloatWidget->getButtonUp()->setEnabled(false);
                m_currentMoreImageNum = 0;
            } else if ((1 == m_imageReader->currentImageNumber()) || (1 == m_currentMoreImageNum)) {
                m_imageReader->jumpToImage(0);
                m_currentMoreImageNum = 0;
                if (m_morePicFloatWidget->getButtonUp()) {
                    m_morePicFloatWidget->getButtonUp()->setEnabled(false);
                }

            } else {
                m_currentMoreImageNum--;
                if (0 == m_imageReader->currentImageNumber()) {
                    m_imageReader->jumpToImage(m_currentMoreImageNum);
                } else {
                    m_imageReader->jumpToImage(m_imageReader->currentImageNumber() - 1);
                }
            }
            m_pixmapItem = nullptr;
            m_pixmapItem = nullptr;
            m_imgSvgItem = nullptr;
            scene()->clear();

            resetTransform();
            QPixmap pixmap = QPixmap::fromImage(m_imageReader->read());
            pixmap.setDevicePixelRatio(devicePixelRatioF());
            m_pixmapItem = new LibGraphicsPixmapItem(pixmap);
            scene()->addItem(m_pixmapItem);
            QRectF rect = m_pixmapItem->boundingRect();
            setSceneRect(rect);
            autoFit();
            if (m_currentMoreImageNum != m_imageReader->currentImageNumber()) {
                m_morePicFloatWidget->setLabelText(QString::number(m_currentMoreImageNum + 1) + "/" + QString::number(m_imageReader->imageCount()));
            } else {
                m_morePicFloatWidget->setLabelText(QString::number(m_imageReader->currentImageNumber() + 1) + "/" + QString::number(m_imageReader->imageCount()));
            }
            //todo ,更新导航栏
            emit UpdateNavImg();
        }
    }
}

void LibImageGraphicsView::slotsDown()
{
    if (m_morePicFloatWidget) {
        if (m_morePicFloatWidget->getButtonUp()) {
            m_morePicFloatWidget->getButtonUp()->setEnabled(true);
        }
        if (m_morePicFloatWidget->getButtonDown()) {
            m_morePicFloatWidget->getButtonDown()->setEnabled(true);
        }

        if (m_pixmapItem && m_imageReader && m_imageReader->imageCount() > 1) {
            if ((m_imageReader->currentImageNumber() == m_imageReader->imageCount() - 1) || m_currentMoreImageNum == m_imageReader->imageCount() - 1) {
                //改为与现在相同按钮点击逻辑相同修改bug62227，最后一张图片时候，向下按钮置灰
                m_morePicFloatWidget->getButtonDown()->setEnabled(false);
                m_currentMoreImageNum = m_imageReader->imageCount() - 1;
            } else if ((m_imageReader->currentImageNumber() == m_imageReader->imageCount() - 2) || (m_currentMoreImageNum == m_imageReader->imageCount() - 2)) {
                m_imageReader->jumpToImage(m_imageReader->imageCount() - 1);
                m_currentMoreImageNum = m_imageReader->imageCount() - 1;
                if (m_morePicFloatWidget->getButtonDown()) {
                    m_morePicFloatWidget->getButtonDown()->setEnabled(false);
                }

            } else {
                m_imageReader->jumpToNextImage();
                m_currentMoreImageNum++;
            }
            //修复bug69273,缩放存在问题
            m_pixmapItem = nullptr;
            m_pixmapItem = nullptr;
            m_imgSvgItem = nullptr;
            scene()->clear();
            resetTransform();
            QPixmap pixmap = QPixmap::fromImage(m_imageReader->read());
            pixmap.setDevicePixelRatio(devicePixelRatioF());
            m_pixmapItem = new LibGraphicsPixmapItem(pixmap);
            scene()->addItem(m_pixmapItem);
            QRectF rect = m_pixmapItem->boundingRect();
            setSceneRect(rect);
            autoFit();
            if (m_currentMoreImageNum != m_imageReader->currentImageNumber()) {
                m_morePicFloatWidget->setLabelText(QString::number(m_currentMoreImageNum + 1) + "/" + QString::number(m_imageReader->imageCount()));
            } else {
                m_morePicFloatWidget->setLabelText(QString::number(m_imageReader->currentImageNumber() + 1) + "/" + QString::number(m_imageReader->imageCount()));
            }

            //todo ,更新导航栏
            emit UpdateNavImg();
        }
    }
}

bool LibImageGraphicsView::slotRotatePixmap(int nAngel)
{
    if (!m_pixmapItem) return false;
    QPixmap pixmap = m_pixmapItem->pixmap();
    QMatrix rotate;
    rotate.rotate(nAngel);

    pixmap = pixmap.transformed(rotate, Qt::SmoothTransformation);
    pixmap.setDevicePixelRatio(devicePixelRatioF());
    scene()->clear();
    resetTransform();
    m_pixmapItem = new LibGraphicsPixmapItem(pixmap);
    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    // Make sure item show in center of view after reload
    QRectF rect = m_pixmapItem->boundingRect();
    //            rect.setHeight(rect.height() + 50);
    setSceneRect(rect);
    //            setSceneRect(m_pixmapItem->boundingRect());
    scene()->addItem(m_pixmapItem);

    autoFit();
    m_rotateAngel += nAngel;

    emit currentThumbnailChanged(pixmap, pixmap.size());
    emit imageChanged(m_path);
    return true;
}

void LibImageGraphicsView::slotRotatePixCurrent()
{
    m_rotateAngel =  m_rotateAngel % 360;
    if (0 != m_rotateAngel) {
        //20211019修改：特殊位置不执行写入操作
        imageViewerSpace::PathType pathType = LibUnionImage_NameSpace::getPathType(m_path);

        if (pathType != imageViewerSpace::PathTypeMTP && pathType != imageViewerSpace::PathTypePTP && //安卓手机
                pathType != imageViewerSpace::PathTypeAPPLE && //苹果手机
                pathType != imageViewerSpace::PathTypeSAFEBOX && //保险箱
                pathType != imageViewerSpace::PathTypeRECYCLEBIN) { //回收站

            disconnect(m_imgFileWatcher, &QFileSystemWatcher::fileChanged, this, &LibImageGraphicsView::onImgFileChanged);
            Libutils::image::rotate(m_path, m_rotateAngel);
            //如果是相册调用，则告知刷新
            if (LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeAlbum) {
                emit ImageEngine::instance()->sigRotatePic(m_path);
            }

            QTimer::singleShot(1000, [ = ] {
                connect(m_imgFileWatcher, &QFileSystemWatcher::fileChanged, this, &LibImageGraphicsView::onImgFileChanged);
            });
            m_rotateAngel = 0;
        }
    }
}

void LibImageGraphicsView::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        emit doubleClicked();
    }

    QGraphicsView::mouseDoubleClickEvent(e);
}

void LibImageGraphicsView::mouseReleaseEvent(QMouseEvent *e)
{
    QGraphicsView::mouseReleaseEvent(e);

    viewport()->setCursor(Qt::ArrowCursor);
    if (e->source() == Qt::MouseEventSynthesizedByQt && m_maxTouchPoints == 1) {
        const QRect &r = visibleImageRect();
        //double left=r.width()+r.x();
        const QRectF &sr = sceneRect();
        //fix 42660 2020/08/14 单指时间在QEvent处理，双指手势通过手势处理。为了解决图片放大后单指滑动手势冲突的问题
        if ((r.width() >= sr.width() && r.height() >= sr.height())) {
            int xpos = e->pos().x() - m_startpointx;
            if (abs(xpos) > 200 && m_startpointx != 0) {
                if (xpos > 0) {
                    emit previousRequested();
                } else {
                    emit nextRequested();
                }
            }
        }
    }
    m_startpointx = 0;
    m_maxTouchPoints = 0;
#ifdef tablet_PC
    if (m_press) {
        emit clicked();
    }
#endif
}

void LibImageGraphicsView::mousePressEvent(QMouseEvent *e)
{
#ifdef tablet_PC
    m_press = true;
#endif
    QGraphicsView::mousePressEvent(e);

    viewport()->unsetCursor();
    viewport()->setCursor(Qt::ArrowCursor);

#ifndef tablet_PC
    emit clicked();
#endif
    m_startpointx = e->pos().x();
}

void LibImageGraphicsView::mouseMoveEvent(QMouseEvent *e)
{

    m_press = false;
    if (!(e->buttons() | Qt::NoButton)) {
        viewport()->setCursor(Qt::ArrowCursor);
        emit mouseHoverMoved();
    } else {
        QGraphicsView::mouseMoveEvent(e);
        viewport()->setCursor(Qt::ClosedHandCursor);

        emit transformChanged();
    }
    emit sigMouseMove();
}

void LibImageGraphicsView::leaveEvent(QEvent *e)
{
//    dApp->getDAppNew()->restoreOverrideCursor();

    QGraphicsView::leaveEvent(e);
}

void LibImageGraphicsView::resizeEvent(QResizeEvent *event)
{
    qDebug() << "---" << __FUNCTION__ << "---" << event->size();
    //20201027曾在右侧浮动窗口，关于多图片
    if (m_morePicFloatWidget) {
        m_morePicFloatWidget->move(this->width() - 80, this->height() / 2 - 50);
    }
    titleBarControl();
    if (!m_isFitWindow) {
        scaleAtPoint(QPoint(0, 0), 1.0);
    }
    QGraphicsView::resizeEvent(event);
//    m_toast->move(width() / 2 - m_toast->width() / 2,
//                  height() - 80 - m_toast->height() / 2 - 11);
}

void LibImageGraphicsView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);
}

void LibImageGraphicsView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mimeData = e->mimeData();
    if (!pluginUtils::base::checkMimeData(mimeData)) {
        return;
    }
    e->accept();
}

void LibImageGraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
//    QPixmap pm(12, 12);
//    QPainter pmp(&pm);
//    //TODO: the transparent box
//    //should not be scaled with the image
//    pmp.fillRect(0, 0, 6, 6, LIGHT_CHECKER_COLOR);
//    pmp.fillRect(6, 6, 6, 6, LIGHT_CHECKER_COLOR);
//    pmp.fillRect(0, 6, 6, 6, DARK_CHECKER_COLOR);
//    pmp.fillRect(6, 0, 6, 6, DARK_CHECKER_COLOR);
//    pmp.end();

    painter->save();
    painter->fillRect(rect, m_backgroundColor);

//    QPixmap currentImage(m_path);
//    if (!currentImage.isNull())
//        painter->fillRect(currentImage.rect(), QBrush(pm));
    painter->restore();
}

int static count = 0;
bool LibImageGraphicsView::event(QEvent *event)
{
    QEvent::Type evType = event->type();
    if (evType == QEvent::TouchBegin || evType == QEvent::TouchUpdate ||
            evType == QEvent::TouchEnd) {
        if (evType == QEvent::TouchBegin) {
            count = 0;
            m_maxTouchPoints = 1;
        }
        if (evType == QEvent::TouchUpdate) {
            QTouchEvent *touchEvent = dynamic_cast<QTouchEvent *>(event);
            QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
            if (touchPoints.size() > count) {
                count = touchPoints.size();
            }
        }
        if (evType == QEvent::TouchEnd) {
            QTouchEvent *touchEvent = dynamic_cast<QTouchEvent *>(event);
            QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();

            if (touchPoints.size() == 1 && count <= 1) {
                //QPointF centerPointOffset = gesture->centerPoint();
                qreal offset = touchPoints.at(0).lastPos().x() - touchPoints.at(0).startPos().x();
                if (qAbs(offset) > 200) {
                    if (offset > 0) {
                        emit previousRequested();
                        qDebug() << "zy------ImageView::event previousRequested";
                    } else {
                        emit nextRequested();
                        qDebug() << "zy------ImageView::event nextRequested";
                    }
                }
            }
        }
        /*lmh0804*/
//        const QRect &r = visibleImageRect();
//        double left = r.width() + r.x();
//        const QRectF &sr = sceneRect();
//        if (r.x() <= 1) {
//            return true;
//        }
//        if (left - sr.width() >= -1 && left - sr.width() <= 1) {
//            return true;
//        }
//        if (r.width() >= sr.width()) {
//            return true;
//        }
    } else if (evType == QEvent::Gesture)
        handleGestureEvent(static_cast<QGestureEvent *>(event));

    return QGraphicsView::event(event);
}

void LibImageGraphicsView::onCacheFinish()
{
    if (m_spinner) {
        m_spinner->stop();
        m_spinner->hide();
    }
    QVariantList vl = m_watcher.result();
    if (vl.length() == 2) {
        const QString path = vl.first().toString();
        if (path == m_path) {
            if (!m_pixmapItem) {
                return;
            }

            QPixmap pixmap = vl.last().value<QPixmap>();
            QPixmap tmpPixmap = pixmap;
            tmpPixmap.setDevicePixelRatio(devicePixelRatioF());
            if (!tmpPixmap.isNull()) {
                pixmap = tmpPixmap;
            }
            if (m_newImageRotateAngle != 0) {
                QMatrix rotate;
                rotate.rotate(m_newImageRotateAngle);
                pixmap = pixmap.transformed(rotate, Qt::SmoothTransformation);
                m_newImageRotateAngle = 0;
            }

            m_pixmapItem->setGraphicsEffect(nullptr);
            m_pixmapItem->setPixmap(pixmap);
            setSceneRect(m_pixmapItem->boundingRect());
            autoFit();
            emit imageChanged(path);
            this->update();
            m_newImageLoadPhase = FullFinish;

            //刷新缩略图
            if (!pixmap.isNull()) {
                emit currentThumbnailChanged(pixmap, pixmap.size());
            }

        }
    }
}

//void ImageView::onThemeChanged(ViewerThemeManager::AppTheme theme)
//{
//    if (theme == ViewerThemeManager::Dark) {
//        m_backgroundColor = utils::common::DARK_BACKGROUND_COLOR;
//        m_loadingIconPath = utils::view::DARK_LOADINGICON;
//    } else {
//        m_backgroundColor = utils::common::LIGHT_BACKGROUND_COLOR;
//        m_loadingIconPath = utils::view::LIGHT_LOADINGICON;
//    }
//    update();
//}

void LibImageGraphicsView::scaleAtPoint(QPoint pos, qreal factor)
{
    // Remember zoom anchor point.
    const QPointF targetPos = pos;
    const QPointF targetScenePos = mapToScene(targetPos.toPoint());

    // Do the scaling.
    setScaleValue(factor);

    // Restore the zoom anchor point.
    //
    // The Basic idea here is we don't care how the scene is scaled or transformed,
    // we just want to restore the anchor point to the target position we've
    // remembered, in the coordinate of the view/viewport.
    const QPointF curPos = mapFromScene(targetScenePos);
    const QPointF centerPos = QPointF(width() / 2.0, height() / 2.0) + (curPos - targetPos);
    const QPointF centerScenePos = mapToScene(centerPos.toPoint());
    centerOn(static_cast<int>(centerScenePos.x()), static_cast<int>(centerScenePos.y()));
}

void LibImageGraphicsView::handleGestureEvent(QGestureEvent *gesture)
{
    if (QGesture *pinch = gesture->gesture(Qt::PinchGesture))
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
}

void LibImageGraphicsView::pinchTriggered(QPinchGesture *gesture)
{
    m_maxTouchPoints = 2;
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        QPoint pos = mapFromGlobal(gesture->centerPoint().toPoint());
        if (abs(gesture->scaleFactor() - 1) > 0.006) {
            scaleAtPoint(pos, gesture->scaleFactor());
        }
    }
#ifndef tablet_PC
    //二指旋转
    if (changeFlags & QPinchGesture::RotationAngleChanged) {
        if (!m_bRoate || m_maxTouchPoints > 2) return;
        //释放手指后旋转的位置未结束不能进行下次旋转
        if (!m_rotateflag) {
            qDebug() << "ratateflag" << gesture->lastRotationAngle();
            gesture->setRotationAngle(gesture->lastRotationAngle());
            return;
        }
        qreal rotationDelta = gesture->rotationAngle() - gesture->lastRotationAngle();
        //防止在旋转过程中触发切换到下一张
        if (abs(gesture->rotationAngle()) > 20) m_bnextflag = false;
        if (abs(rotationDelta) > 0.2) {
            m_rotateAngelTouch = gesture->rotationAngle();
            rotate(rotationDelta);
        }
    }
//    if (gesture->state() == Qt::GestureFinished) {

//        QPointF centerPointOffset = gesture->centerPoint();
//        qreal offset = centerPointOffset.x() - m_centerPoint.x();
//        if (qAbs(offset) > 200) {
//            if (offset > 0) {
//                emit previousRequested();
//                qDebug() << "zy------ImageView::pinchTriggered previousRequested";
//            } else {
//                emit nextRequested();
//                qDebug() << "zy------ImageView::pinchTriggered nextRequested";
//            }
//        }
//        m_isFirstPinch = false;
//    }
#endif
    if (changeFlags & QPinchGesture::CenterPointChanged) {
        if (!m_isFirstPinch) {
            m_centerPoint = gesture->centerPoint();
            m_isFirstPinch = true;
        }
    }

    if (gesture->state() == Qt::GestureFinished) {
        m_isFirstPinch = false;
        gesture->setCenterPoint(m_centerPoint);
        //旋转松开手势操作
        // m_rotateAngelTouch = m_rotateAngelTouch % 360;
        //int abs(m_rotateAngelTouch);
        if (!m_bRoate) return;
        m_rotateflag = false;
        QPropertyAnimation *animation = new QPropertyAnimation(this, "rotation");
        animation->setDuration(200);
        if (m_rotateAngelTouch < 0) m_rotateAngelTouch += 360;
        qreal endvalue;
        if (abs(0 - abs(m_rotateAngelTouch)) <= 10) {
            endvalue = 0;
        } else if (abs(360 - abs(m_rotateAngelTouch)) <= 10) {
            endvalue = 0;
        } else if (abs(90 - abs(m_rotateAngelTouch)) <= 10) {
            endvalue = 90;
        } else if (abs(180 - abs(m_rotateAngelTouch)) <= 10) {
            endvalue = 180;
        } else if (abs(270 - abs(m_rotateAngelTouch)) <= 10) {
            endvalue = 270;
        } else {
            endvalue = 0;
        }
//         if(!m_bRoate) endvalue = 0;
        m_endvalue = endvalue;
        qreal startvalue;
        if (abs(m_rotateAngelTouch - endvalue) > 180) {
            startvalue = m_rotateAngelTouch - 360;
        } else {
            startvalue = m_rotateAngelTouch;
        }
        animation->setStartValue(startvalue);
        animation->setEndValue(endvalue);
        // qDebug()<<"angle finish" <<m_rotateAngelTouch << endvalue;
        connect(animation, &QVariantAnimation::valueChanged, [ = ](const QVariant & value) {
            qreal angle = value.toReal() - m_rotateAngelTouch;
            m_rotateAngelTouch = value.toReal();
            if (static_cast<int>(value.toReal()) != static_cast<int>(endvalue) /*|| m_imgSvgItem*/)
                this->rotate(angle);
            //setPixmap(pixmap.transformed(t));
        });
        // animation->setLoopCount(1); //旋转次数
        connect(animation, SIGNAL(finished()), this, SLOT(OnFinishPinchAnimal()));
        animation->start(QAbstractAnimation::KeepWhenStopped);
        qDebug() << "finish";

    }
}

void LibImageGraphicsView::OnFinishPinchAnimal()
{
    m_rotateflag = true;
    m_bnextflag = true;
    m_rotateAngelTouch = 0;
//    if(m_imgSvgItem)
//    {
//      //  reloadSvgPix(m_path,90,false);
//        m_rotateAngel += m_endvalue;
//        dApp->m_imageloader->updateImageLoader(QStringList(m_path), true,static_cast<int>(m_endvalue));
//        emit dApp->signalM->sigUpdateThunbnail(m_path);
//        return;
//    }
    if (!m_pixmapItem) return;
    //QStranform旋转到180度有问题，暂未解决，因此动画结束后旋转Pixmap到180
    QPixmap pixmap;
    pixmap = m_pixmapItem->pixmap();
    QMatrix rotate;
    rotate.rotate(m_endvalue);

    pixmap = pixmap.transformed(rotate, Qt::FastTransformation);
    pixmap.setDevicePixelRatio(devicePixelRatioF());
    scene()->clear();
    resetTransform();
    m_pixmapItem = new LibGraphicsPixmapItem(pixmap);
    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    // Make sure item show in center of view after reload
    QRectF rect = m_pixmapItem->boundingRect();
    //            rect.setHeight(rect.height() + 50);
    setSceneRect(rect);
    //            setSceneRect(m_pixmapItem->boundingRect());

    scene()->addItem(m_pixmapItem);
    scale(m_scal, m_scal);
    if (m_bRoate) {
        m_rotateAngel += m_endvalue;
        if (m_endvalue > 0) {
            emit gestureRotate(static_cast<int>(0));
            emit currentThumbnailChanged(pixmap, pixmap.size());
            emit UpdateNavImg();
        }
    }
    qDebug() << m_endvalue;
}

void LibImageGraphicsView::wheelEvent(QWheelEvent *event)
{
    QFileInfo file(m_path);
    if (!file.exists()) {
        event->accept();
    } else {

        qreal factor = qPow(1.2, event->delta() / 240.0);
        qDebug() << factor;
        scaleAtPoint(event->pos(), factor);

        event->accept();
    }
//    qDebug() << "---" << __FUNCTION__ << "---" << this->sceneRect();
}

//CFileWatcher::CFileWatcher(QObject *parent): QThread(parent)
//{
//    _handleId = inotify_init();
//}

//CFileWatcher::~CFileWatcher()
//{
//    clear();
//}

//bool CFileWatcher::isVaild()
//{
//    return (_handleId != -1);
//}

//void CFileWatcher::addWather(const QString &path)
//{
//    QMutexLocker loker(&_mutex);
//    if (!isVaild())
//        return;
//    QFileInfo info(path);
//    if (!info.exists() || !info.isFile()) {
//        return;
//    }
//    if (watchedFiles.find(path) != watchedFiles.end()) {
//        return;
//    }
//    std::string sfile = path.toStdString();
//    int fileId = inotify_add_watch(_handleId, sfile.c_str(), IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF);
//    watchedFiles.insert(path, fileId);
//    watchedFilesId.insert(fileId, path);
//    if (!_running) {
//        _running = true;
//        start();
//    }
//}

//void CFileWatcher::removePath(const QString &path)
//{
//    QMutexLocker loker(&_mutex);
//    if (!isVaild())
//        return;
//    auto itf = watchedFiles.find(path);
//    if (itf != watchedFiles.end()) {
//        inotify_rm_watch(_handleId, itf.value());
//        watchedFilesId.remove(itf.value());
//        watchedFiles.erase(itf);
//    }
//}

//void CFileWatcher::clear()
//{
//    QMutexLocker loker(&_mutex);
//    for (auto it : watchedFiles) {
//        inotify_rm_watch(_handleId, it);
//    }
//    watchedFilesId.clear();
//    watchedFiles.clear();
//}

//void CFileWatcher::run()
//{
//    doRun();
//}

//void CFileWatcher::doRun()
//{
//    if (!isVaild())
//        return;

//    char name[1024];
//    auto freadsome = [ = ](void *dest, size_t remain, FILE * file) {
//        char *offset = reinterpret_cast<char *>(dest);
//        while (remain) {
//            if (file == nullptr)
//                return -1;
//            size_t n = fread(offset, 1, remain, file);
//            if (n == 0) {
//                return -1;
//            }

//            remain -= n;
//            offset += n;
//        }
//        return 0;
//    };

//    FILE *watcher_file = fdopen(_handleId, "r");

//    while (true) {
//        inotify_event event;
//        if (-1 == freadsome(&event, sizeof(event), watcher_file)) {
//            qWarning() << "------------- freadsome error !!!!!---------- ";
//            break;
//        }
//        if (event.len) {
//            freadsome(name, event.len, watcher_file);
//        } else {
//            QMutexLocker loker(&_mutex);
//            auto itf = watchedFilesId.find(event.wd);
//            if (itf != watchedFilesId.end()) {
//                //qDebug() << "file = " << itf.value() << " event.wd = " << event.wd << "event.mask = " << event.mask;

//                if (event.mask & IN_MODIFY) {
//                    emit fileChanged(itf.value(), EFileModified);
//                } else if (event.mask & IN_MOVE_SELF) {
//                    emit fileChanged(itf.value(), EFileMoved);
//                } else if (event.mask & IN_DELETE_SELF) {
//                    emit fileChanged(itf.value(), EFileMoved);
//                }
//            }
//        }
//    }
//}
