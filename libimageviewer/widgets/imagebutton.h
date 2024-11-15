// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEBUTTON_H
#define IMAGEBUTTON_H

#include <dtkcore_global.h>

#if DTK_VERSION < DTK_VERSION_CHECK(6, 0, 0, 0)
#include <dimagebutton.h>
#else
#include <DToolButton>
#endif

#include <DLabel>

class QEvent;
class QEnterEvent;
DWIDGET_USE_NAMESPACE

#if DTK_VERSION < DTK_VERSION_CHECK(6, 0, 0, 0)
class ImageButton : public DImageButton
#else
class ImageButton : public DToolButton
#endif
{
    Q_OBJECT
public:
    explicit ImageButton(QWidget *parent = nullptr);
    explicit ImageButton(const QString &normalPic,
                         const QString &hoverPic,
                         const QString &pressPic,
                         const QString &disablePic,
                         QWidget *parent = nullptr);

    void setDisabled(bool d);

    void setTooltipVisible(bool visible);

    inline const QString getDisablePic() const { return m_disablePic_; }

    Q_SIGNAL void mouseLeave();

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent *e) Q_DECL_OVERRIDE;
#else
    void enterEvent(QEnterEvent *e) Q_DECL_OVERRIDE;
#endif

    bool event(QEvent *e) Q_DECL_OVERRIDE;

private:
    void showTooltip(const QPoint &gPos);

    bool m_tooltipVisiable;
    QString m_disablePic_;
};

#endif  // IMAGEBUTTON_H
