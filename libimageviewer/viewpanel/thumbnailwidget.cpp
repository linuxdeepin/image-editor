// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDebug>
#include <QPainter>
#include <QVBoxLayout>
#include <QGestureEvent>

#include <DSuggestButton>

#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DLabel>
#include <QImageReader>

#include "thumbnailwidget.h"
#include "unionimage/baseutils.h"
#include "accessibility/ac-desktop-define.h"

namespace {
const QSize THUMBNAIL_BORDERSIZE = QSize(100, 100);
const QSize THUMBNAIL_SIZE = QSize(98, 98);
const QString ICON_IMPORT_PHOTO_DARK = ":/dark/images/icon_import_photo dark.svg";
const QString ICON_IMPORT_PHOTO_LIGHT = ":/light/images/icon_import_photo.svg";
}  // namespace

ThumbnailWidget::ThumbnailWidget(const QString &darkFile, const QString &lightFile, QWidget *parent)
    : ThemeWidget(darkFile, lightFile, parent),
      m_picString("")
{
    qDebug() << "Initializing ThumbnailWidget with dark file:" << darkFile << "and light file:" << lightFile;
#ifdef OPENACCESSIBLE
    setObjectName(Thumbnail_Widget);
    setAccessibleName(Thumbnail_Widget);
    qDebug() << "Accessibility names set for ThumbnailWidget";
#endif
    this->setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PanGesture);
    qDebug() << "Touch gestures initialized";

    //修复style问题
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        m_picString = ICON_IMPORT_PHOTO_DARK;
        m_theme = true;
        qDebug() << "Using dark theme icon:" << ICON_IMPORT_PHOTO_DARK;
    } else {
        m_picString = ICON_IMPORT_PHOTO_LIGHT;
        m_theme = false;
        qDebug() << "Using light theme icon:" << ICON_IMPORT_PHOTO_LIGHT;
    }

    QPixmap logo_pix = Libutils::base::renderSVG(m_picString, THUMBNAIL_SIZE);
    m_logo = logo_pix;
    qDebug() << "Logo rendered with size:" << THUMBNAIL_SIZE;

    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
    this, [ = ]() {
        DGuiApplicationHelper::ColorType themeType =
            DGuiApplicationHelper::instance()->themeType();
        m_picString = "";
        if (themeType == DGuiApplicationHelper::DarkType) {
            m_picString = ICON_IMPORT_PHOTO_DARK;
            m_theme = true;
            qDebug() << "Theme changed to dark";
        } else {
            m_picString = ICON_IMPORT_PHOTO_LIGHT;
            m_theme = false;
            qDebug() << "Theme changed to light";
        }

        //修复style风格错误
        m_logo = Libutils::base::renderSVG(m_picString, THUMBNAIL_SIZE);
        if (m_isDefaultThumbnail) {
            //这里之前修复风格错误,导致bug68248,现在已经修复
            m_defaultImage = m_logo;
            qDebug() << "Updated default thumbnail with new theme logo";
        }
        update();
    });

    setMouseTracking(true);
    m_thumbnailLabel = new QLbtoDLabel(this);
    m_thumbnailLabel->setFixedSize(THUMBNAIL_BORDERSIZE);
    qDebug() << "Thumbnail label created with size:" << THUMBNAIL_BORDERSIZE;
    onThemeChanged(DGuiApplicationHelper::instance()->themeType());

#ifndef LITE_DIV
    m_tips = new QLabel(this);
    m_tips->setObjectName("ThumbnailTips");
    m_tips->setText(tr("No image files found"));
    qDebug() << "Tips label created with text: No image files found";
#else
    m_tips = new DLabel(this);
    m_tips->setText(tr("Image file not found"));
    m_tips->show();

//    DSuggestButton *button = new DSuggestButton(tr("Open Image"), this);
//    button->setFixedWidth(302);
//    button->setFixedHeight(36);
    // button->setShortcut(QKeySequence("Ctrl+O"));
#ifdef OPENACCESSIBLE
    m_thumbnailLabel->setObjectName(Thumbnail_Label);
    m_thumbnailLabel->setAccessibleName(Thumbnail_Label);
    m_tips->setObjectName(NOT_FOUND_IMAGE);
    m_tips->setAccessibleName(NOT_FOUND_IMAGE);
//    button->setObjectName(OPEN_IMAGE);
//    button->setAccessibleName(OPEN_IMAGE);
#endif
//    connect(button, &DSuggestButton::clicked, this, &ThumbnailWidget::openImageInDialog);

//    connect(dApp->signalM, &SignalManager::usbOutIn, this, [ = ](bool visible) {
//        if (button->isVisible())
//            return;
//        if (visible) {
//            if (m_usb) {
//                button->hide();
//                tips->hide();
//                m_usb = false;
//            } else {
//                button->show();
//                tips->hide();
//            }
//        } else {
//            button->hide();
//            tips->show();
//            m_usb = true;
//        }
//    });
//    connect(dApp->signalM, &SignalManager::picNotExists, this, [ = ](bool visible) {
//        if (visible) {
//            button->hide();
//            tips->show();
//            //当显示图片是不存在时，应该让标题栏不透明
//            dApp->signalM->sigImageOutTitleBar(false);
//        } else {
//            button->show();
//            tips->hide();
//        }
//    });
#endif
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(m_thumbnailLabel, 0, Qt::AlignCenter);
    layout->addSpacing(9);
    layout->addWidget(m_tips, 0, Qt::AlignCenter);
    layout->addStretch();
    setLayout(layout);
    qDebug() << "Layout initialized with thumbnail label and tips";

    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
                     this, &ThumbnailWidget::onThemeChanged);
}

void ThumbnailWidget::onThemeChanged(DGuiApplicationHelper::ColorType theme)
{
    qDebug() << "Theme changed to:" << (theme == DGuiApplicationHelper::ColorType::DarkType ? "Dark" : "Light");
    if (theme == DGuiApplicationHelper::ColorType::DarkType) {
        m_inBorderColor = Libutils::common::DARK_BORDER_COLOR;
        if (m_isDefaultThumbnail)
            m_defaultImage = m_logo;
    } else {
        m_inBorderColor = Libutils::common::LIGHT_BORDER_COLOR;
        if (m_isDefaultThumbnail)
            m_defaultImage = m_logo;
    }

    ThemeWidget::onThemeChanged(theme);
    update();
}

void ThumbnailWidget::setThumbnailImageAndText(const QPixmap thumbnail, DisplayType type)
{
    qDebug() << "Setting thumbnail image and text, type:" << type;
    switch (type) {
    default:
        break;
    case DamageType:
        if (thumbnail.isNull()) {
            m_defaultImage = m_logo;
            m_isDefaultThumbnail = true;
            qDebug() << "Using default logo for damaged image";
        } else {
            m_defaultImage = thumbnail;
            m_isDefaultThumbnail = false;
            qDebug() << "Using provided thumbnail for damaged image";
        }
        m_tips->setText(tr("Image file not found"));
        DFontSizeManager::instance()->bind(m_tips, DFontSizeManager::T6);
        m_tips->setForegroundRole(DPalette::TextTips);
        break;
    case CannotReadType:
        m_defaultImage = thumbnail;
        m_isDefaultThumbnail = false;
        m_tips->setText(tr("You have no permission to view the file"));
        DFontSizeManager::instance()->bind(m_tips, DFontSizeManager::T6);
        m_tips->setForegroundRole(DPalette::TextTitle);
        qDebug() << "Setting cannot read type message";
        break;
    }

    update();
}

void ThumbnailWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_defaultImage.isNull() && !m_isDefaultThumbnail) {
        qDebug() << "Painting empty thumbnail";
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        QIcon m_icon(m_defaultImage);
        m_icon.paint(&painter, QRect(1, 1, 1, 1));
        return;
    }

    if (m_defaultImage.isNull() && m_isDefaultThumbnail) {
        qDebug() << "Using default logo for empty thumbnail";
        m_defaultImage = m_logo;
    }

    if (m_defaultImage.size() != THUMBNAIL_SIZE) {
        qDebug() << "Scaling thumbnail to size:" << THUMBNAIL_SIZE;
        m_defaultImage =
            m_defaultImage.scaled(THUMBNAIL_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QPoint startPoint = mapToParent(QPoint(m_thumbnailLabel->x(), m_thumbnailLabel->y()));
    QPoint imgStartPoint = QPoint(startPoint.x() + (THUMBNAIL_SIZE.width() - 128) / 2 + 1,
                                  startPoint.y() + (THUMBNAIL_SIZE.height() - 128) / 2 + 1);
    QRect imgRect = QRect(imgStartPoint.x(), imgStartPoint.y(), 128, 128);
    qDebug() << "Painting thumbnail at rect:" << imgRect;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QIcon m_icon(m_defaultImage);
    m_icon.paint(&painter, imgRect);
}

void ThumbnailWidget::mouseReleaseEvent(QMouseEvent *e)
{
    QWidget::mouseReleaseEvent(e);
    if (e->source() == Qt::MouseEventSynthesizedByQt && m_maxTouchPoints == 1) {
        int offset = e->globalPos().x() - m_startx;
        if (qAbs(offset) > 200) {
            if (offset > 0) {
                qDebug() << "Swipe right detected, emitting previousRequested signal";
                emit previousRequested();
            } else {
                qDebug() << "Swipe left detected, emitting nextRequested signal";
                emit nextRequested();
            }
        }
    }
    m_startx = 0;
}

void ThumbnailWidget::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
    m_startx = e->globalPos().x();
    qDebug() << "Mouse press at x position:" << m_startx;
}

void ThumbnailWidget::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit sigMouseMove();
}

void ThumbnailWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    //非平板才能双击,其他是单击全屏
    if (e->button() == Qt::LeftButton) {
        qDebug() << "Double click detected, emitting showfullScreen signal";
        emit showfullScreen();
    }
    ThemeWidget::mouseDoubleClickEvent(e);
}

void ThumbnailWidget::handleGestureEvent(QGestureEvent *gesture)
{
    if (QGesture *pinch = gesture->gesture(Qt::PinchGesture)) {
        qDebug() << "Handling pinch gesture";
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
}

bool ThumbnailWidget::event(QEvent *event)
{
    QEvent::Type evType = event->type();
    if (evType == QEvent::TouchBegin || evType == QEvent::TouchUpdate ||
            evType == QEvent::TouchEnd) {
        if (evType == QEvent::TouchBegin) {
            qDebug() << "Touch begin event detected";
            m_maxTouchPoints = 1;
        }
    } else if (event->type() == QEvent::Gesture) {
        qDebug() << "Gesture event detected";
        handleGestureEvent(static_cast<QGestureEvent *>(event));
    }
    return QWidget::event(event);
}

void ThumbnailWidget::pinchTriggered(QPinchGesture *gesture)
{
    Q_UNUSED(gesture);
    m_maxTouchPoints = 2;
    qDebug() << "Pinch gesture triggered, max touch points set to 2";
}

ThumbnailWidget::~ThumbnailWidget()
{
    qDebug() << "Destroying ThumbnailWidget";
}

void ThumbnailWidget::wheelEvent(QWheelEvent *event)
{
    if ((event->modifiers() == Qt::ControlModifier)) {
        if (event->angleDelta().y() > 0) {
            qDebug() << "Control + wheel up detected, emitting previousRequested signal";
            emit previousRequested();
        } else if (event->angleDelta().y() < 0) {
            qDebug() << "Control + wheel down detected, emitting nextRequested signal";
            emit nextRequested();
        }
    }
}
