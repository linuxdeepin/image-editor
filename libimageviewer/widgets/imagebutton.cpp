// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagebutton.h"

#include <QApplication>
#include <QEvent>
#include <QEnterEvent>
#include <QFile>
#include <QHelpEvent>
#include <QDebug>
#include <QHBoxLayout>
#include <QTimer>
#include <QScreen>

#include <DLabel>
#include <DFrame>

DWIDGET_USE_NAMESPACE
typedef DFrame QFrToDFrame;
typedef DLabel QLbtoDLabel;

// comaptible alias
#if DTK_VERSION >= DTK_VERSION_CHECK(6, 0, 0, 0)
using DImageButton = DToolButton;
#endif

ImageButton::ImageButton(QWidget *parent)
#if DTK_VERSION < DTK_VERSION_CHECK(6, 0, 0, 0)
    : DImageButton(parent)
#else
    : DToolButton(parent)
#endif
    , m_tooltipVisiable(false)
{
}

ImageButton::ImageButton(
    const QString &normalPic, const QString &hoverPic, const QString &pressPic, const QString &disablePic, QWidget *parent)
#if DTK_VERSION < DTK_VERSION_CHECK(6, 0, 0, 0)
    : DImageButton(normalPic, hoverPic, pressPic, parent)
#else
    : DToolButton(parent)
#endif
    , m_tooltipVisiable(false)
    , m_disablePic_(disablePic)
{
#if DTK_VERSION >= DTK_VERSION_CHECK(6, 0, 0, 0)
    setIcon(QIcon(normalPic));
#endif
}

void ImageButton::setDisabled(bool d)
{
    DImageButton::setDisabled(d);
}

bool ImageButton::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        if (QHelpEvent *he = static_cast<QHelpEvent *>(e)) {
            showTooltip(he->globalPos());

            return false;
        }
    } else if (e->type() == QEvent::Leave) {
        emit mouseLeave();
        DImageButton::leaveEvent(e);
    } else if (e->type() == QEvent::MouseButtonPress) {
        emit mouseLeave();
    }

    return DImageButton::event(e);
}

void ImageButton::setTooltipVisible(bool visible)
{
    m_tooltipVisiable = visible;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void ImageButton::enterEvent(QEvent *e)
#else
void ImageButton::enterEvent(QEnterEvent *e)
#endif
{
    if (isEnabled()) {
        DImageButton::enterEvent(e);
    }
}

void ImageButton::showTooltip(const QPoint &gPos)
{
    if (toolTip().trimmed().isEmpty() || m_tooltipVisiable) {
        return;
    } else {
        m_tooltipVisiable = true;
    }

    QFrToDFrame *tf = new QFrToDFrame();

    tf->setWindowFlags(Qt::ToolTip);
    tf->setAttribute(Qt::WA_TranslucentBackground);
    QLbtoDLabel *tl = new QLbtoDLabel(tf);
    tl->setObjectName("ButtonTooltip");
    tl->setText(toolTip());
    QHBoxLayout *layout = new QHBoxLayout(tf);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tl);

    tf->show();
    QRect dr = qApp->primaryScreen()->geometry();
    int y = gPos.y() + tf->height();
    if (y > dr.y() + dr.height()) {
        y = gPos.y() - tf->height() - 10;
    }
    tf->move(gPos.x() - tf->width() / 3, y - tf->height() / 3);

    QTimer::singleShot(5000, tf, SLOT(deleteLater()));

    connect(tf, &QFrToDFrame::destroyed, this, [=] { m_tooltipVisiable = false; });

    connect(this, &ImageButton::mouseLeave, tf, &QFrToDFrame::deleteLater);
}
