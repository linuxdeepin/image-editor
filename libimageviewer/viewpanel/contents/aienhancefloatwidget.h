// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AIENHANCEFLOATWIDGET_H
#define AIENHANCEFLOATWIDGET_H

#include <DFloatingWidget>
#include <DIconButton>

DWIDGET_USE_NAMESPACE

class AIEnhanceFloatWidget : public DFloatingWidget
{
    Q_OBJECT
public:
    explicit AIEnhanceFloatWidget(QWidget *parent = nullptr);

    Q_SIGNAL void reset();
    Q_SIGNAL void save();
    Q_SIGNAL void saveAs();

private:
    void initButtton();

private:
    DIconButton *resetBtn = nullptr;
    DIconButton *saveBtn = nullptr;
    DIconButton *saveAsBtn = nullptr;
};

#endif  // AIENHANCEFLOATWIDGET_H
