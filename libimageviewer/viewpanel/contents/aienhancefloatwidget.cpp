// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aienhancefloatwidget.h"

#include <QVBoxLayout>

#include <DAnchors>

const int FLOAT_WDITH = 72;
const int FLOAT_HEIGHT = 122;  // 172, 移除保存按钮
const int FLOAT_RIGHT_MARGIN = 15;
const int FLOAT_RADIUS = 18;
const QSize FLOAT_BTN_SIZE = QSize(40, 40);
const QSize FLOAT_ICON_SIZE = QSize(20, 20);

AIEnhanceFloatWidget::AIEnhanceFloatWidget(QWidget *parent)
    : DFloatingWidget(parent)
{
    setObjectName("AIEnhanceFloatWidget");
    setFixedSize(FLOAT_WDITH, FLOAT_HEIGHT);
    setFramRadius(FLOAT_RADIUS);
    setBlurBackgroundEnabled(true);
    initButtton();

    if (parent) {
        DAnchorsBase::setAnchor(this, Qt::AnchorRight, parent, Qt::AnchorRight);
        DAnchorsBase::setAnchor(this, Qt::AnchorVerticalCenter, parent, Qt::AnchorVerticalCenter);
        DAnchorsBase *anchor = DAnchorsBase::getAnchorBaseByWidget(this);
        if (anchor) {
            anchor->setRightMargin(FLOAT_RIGHT_MARGIN);
        }
    }
}

void AIEnhanceFloatWidget::initButtton()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(10);

    resetBtn = new DIconButton(this);
    resetBtn->setFixedSize(FLOAT_BTN_SIZE);
    resetBtn->setIcon(QIcon::fromTheme("dcc_reset"));
    resetBtn->setIconSize(FLOAT_ICON_SIZE);
    resetBtn->setToolTip(tr("Reprovision"));
    mainLayout->addWidget(resetBtn);
    connect(resetBtn, &DIconButton::clicked, this, &AIEnhanceFloatWidget::reset);

    // 屏蔽Save按钮
#if 0
    saveBtn = new DIconButton(this);
    saveBtn->setFixedSize(FLOAT_BTN_SIZE);
    saveBtn->setIcon(QIcon::fromTheme("dcc_save"));
    saveBtn->setIconSize(FLOAT_ICON_SIZE);
    saveBtn->setToolTip(tr("Save"));
    mainLayout->addWidget(saveBtn);
    connect(saveBtn, &DIconButton::clicked, this, &AIEnhanceFloatWidget::save);
#endif

    saveAsBtn = new DIconButton(this);
    saveAsBtn->setFixedSize(FLOAT_BTN_SIZE);
    saveAsBtn->setIcon(QIcon::fromTheme("dcc_file_save_as"));
    saveAsBtn->setIconSize(FLOAT_ICON_SIZE);
    saveAsBtn->setToolTip(tr("Save as"));
    mainLayout->addWidget(saveAsBtn);
    connect(saveAsBtn, &DIconButton::clicked, this, &AIEnhanceFloatWidget::saveAs);

    setLayout(mainLayout);
}
