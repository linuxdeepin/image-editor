// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lockwidget.h"

#include <QVBoxLayout>
#include <DGuiApplicationHelper>
#include <QGestureEvent>
#include <QDebug>

#include "unionimage/baseutils.h"

#include "accessibility/ac-desktop-define.h"

const QString ICON_PIXMAP_DARK = ":/dark/images/picture damaged_dark.svg";
const QString ICON_PIXMAP_LIGHT = ":/light/images/picture damaged_light.svg";
const QSize THUMBNAIL_SIZE = QSize(151, 151);
LockWidget::LockWidget(const QString &darkFile,
                       const QString &lightFile, QWidget *parent)
    : ThemeWidget(darkFile, lightFile, parent),
      m_picString("")
{
    qDebug() << "Initializing LockWidget with dark file:" << darkFile << "and light file:" << lightFile;
    setMouseTracking(true);
    this->setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PanGesture);
    qDebug() << "Touch gestures initialized";

    //修复style问题
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        m_picString = ICON_PIXMAP_DARK;
        m_theme = true;
        qDebug() << "Using dark theme icon:" << ICON_PIXMAP_DARK;
    } else {
        m_picString = ICON_PIXMAP_LIGHT;
        m_theme = false;
        qDebug() << "Using light theme icon:" << ICON_PIXMAP_LIGHT;
    }

    m_bgLabel = new DLabel(this);
    m_bgLabel->setFixedSize(151, 151);
    m_bgLabel->setObjectName("BgLabel");
    qDebug() << "Background label created with size:" << QSize(151, 151);

#ifdef OPENACCESSIBLE
    setObjectName(Lock_Widget);
    setAccessibleName(Lock_Widget);
    m_bgLabel->setAccessibleName("BgLabel");
    qDebug() << "Accessibility names set";
#endif

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
    this, [ = ]() {
        DGuiApplicationHelper::ColorType themeType =
            DGuiApplicationHelper::instance()->themeType();
        m_picString = "";
        if (themeType == DGuiApplicationHelper::DarkType) {
            m_picString = ICON_PIXMAP_DARK;
            m_theme = true;
            qDebug() << "Theme changed to dark";
        } else {
            m_picString = ICON_PIXMAP_LIGHT;
            m_theme = false;
            qDebug() << "Theme changed to light";
        }

        QPixmap logo_pix = Libutils::base::renderSVG(m_picString, THUMBNAIL_SIZE);
        if (m_bgLabel) {
            m_bgLabel->setPixmap(logo_pix);
            qDebug() << "Updated background label with new theme icon";
        }
    });

    m_lockTips = new DLabel(this);
    m_lockTips->setObjectName("LockTips");
    m_lockTips->setVisible(false);
    setContentText(tr("You have no permission to view the image"));
    qDebug() << "Lock tips label created and initialized";

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addStretch(1);
    QPixmap logo_pix = Libutils::base::renderSVG(m_picString, THUMBNAIL_SIZE);
    m_bgLabel->setPixmap(logo_pix);
    layout->addWidget(m_bgLabel, 0, Qt::AlignHCenter);
    layout->addStretch(1);
    qDebug() << "Layout initialized with background label";

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::paletteTypeChanged, this,
            &LockWidget::onThemeChanged);
}

void LockWidget::setContentText(const QString &text)
{
    qDebug() << "Setting lock tips text:" << text;
    m_lockTips->setText(text);
    int textHeight = Libutils::base::stringHeight(m_lockTips->font(),
                                                  m_lockTips->text());
    m_lockTips->setMinimumHeight(textHeight + 2);
    qDebug() << "Lock tips height set to:" << (textHeight + 2);
}

void LockWidget::handleGestureEvent(QGestureEvent *gesture)
{
    if (QGesture *pinch = gesture->gesture(Qt::PinchGesture)) {
        qDebug() << "Handling pinch gesture";
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
}

void LockWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    //非平板才能双击,其他是单击全屏
    if (e->button() == Qt::LeftButton) {
        qDebug() << "Double click detected, emitting showfullScreen signal";
        emit showfullScreen();
    }
    ThemeWidget::mouseDoubleClickEvent(e);
}

void LockWidget::mouseReleaseEvent(QMouseEvent *e)
{
#if 0
    //平板单击全屏需求
    if (dApp->isPanelDev()) {
        int xpos = e->globalPos().x() - m_startx;
        if ((QDateTime::currentMSecsSinceEpoch() - m_clickTime) < 200 && abs(xpos) < 50) {
            m_clickTime = QDateTime::currentMSecsSinceEpoch();
            emit showfullScreen();
        }
    }
#endif
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

void LockWidget::mousePressEvent(QMouseEvent *e)
{
#if 0
    //平板单击全屏需求(暂时屏蔽)
    if (dApp->isPanelDev()) {
        m_clickTime = QDateTime::currentMSecsSinceEpoch();
    }
#endif
    QWidget::mousePressEvent(e);
    m_startx = e->globalPos().x();
    qDebug() << "Mouse press at x position:" << m_startx;
}

void LockWidget::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit sigMouseMove();
}

bool LockWidget::event(QEvent *event)
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

void LockWidget::pinchTriggered(QPinchGesture *gesture)
{
    Q_UNUSED(gesture);
    m_maxTouchPoints = 2;
    qDebug() << "Pinch gesture triggered, max touch points set to 2";
}

void LockWidget::onThemeChanged(DGuiApplicationHelper::ColorType theme)
{
    qDebug() << "Theme changed to:" << (theme == DGuiApplicationHelper::DarkType ? "Dark" : "Light");
    ThemeWidget::onThemeChanged(theme);
    update();
}

LockWidget::~LockWidget()
{
    qDebug() << "Destroying LockWidget";
    if (m_bgLabel) {
        m_bgLabel->deleteLater();
        m_bgLabel = nullptr;
        qDebug() << "Background label deleted";
    }
    if (m_lockTips) {
        m_lockTips->deleteLater();
        m_lockTips = nullptr;
        qDebug() << "Lock tips label deleted";
    }
}

void LockWidget::wheelEvent(QWheelEvent *event)
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
