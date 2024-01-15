// SPDX-FileCopyrightText: 2020 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PRINTHELPER_H
#define PRINTHELPER_H

#include <QObject>

#include <dprintpreviewwidget.h>
#include <dprintpreviewdialog.h>
DWIDGET_USE_NAMESPACE

//重构printhelper，因为dtk更新
//绘制图片处理类
class RequestedSlot : public QObject
{
    Q_OBJECT

public:
    explicit RequestedSlot(QObject *parent = nullptr);
    ~RequestedSlot();

private slots:
    void paintRequestSync(DPrinter *_printer);

public:
    QStringList m_paths;
    QList<QImage> m_imgs;
};

class PrintHelper : public QObject
{
    Q_OBJECT

public:
    static PrintHelper *getIntance();
    ~PrintHelper();

    void showPrintDialog(const QStringList &paths, QWidget *parent = nullptr);

    RequestedSlot *m_re = nullptr;

private:
    explicit PrintHelper(QObject *parent = nullptr);
    static PrintHelper *m_Printer;
};

#endif  // PRINTHELPER_H
