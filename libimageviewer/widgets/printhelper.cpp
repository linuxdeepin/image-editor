// SPDX-FileCopyrightText: 2020 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "printhelper.h"
#include "service/permissionconfig.h"

#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>
#include <QPrinter>
#include <QPainter>
#include <QToolBar>
#include <QCoreApplication>
#include <QImageReader>
#include <QDebug>

#include <DApplication>
#include <dprintpreviewsettinginfo.h>
#include <dtkwidget_config.h>

#ifdef USE_UNIONIMAGE
#include "printhelper.h"
#include "unionimage/unionimage.h"
#endif

PrintHelper *PrintHelper::m_Printer = nullptr;

PrintHelper *PrintHelper::getIntance()
{
    if (!m_Printer) {
        m_Printer = new PrintHelper();
    }
    return m_Printer;
}

PrintHelper::PrintHelper(QObject *parent)
    : QObject(parent)
{
    m_re = new RequestedSlot(this);
}

PrintHelper::~PrintHelper()
{
    m_re->deleteLater();
}

void PrintHelper::showPrintDialog(const QStringList &paths, QWidget *parent)
{
    if (!PermissionConfig::instance()->isPrintable()) {
        return;
    }

    m_re->m_paths.clear();
    m_re->m_imgs.clear();

    m_re->m_paths = paths;
    QStringList tempExsitPaths;  // 保存存在的图片路径
    for (const QString &path : paths) {
        QString errMsg;
        QImageReader imgReadreder(path);
        if (imgReadreder.imageCount() > 1) {
            for (int imgindex = 0; imgindex < imgReadreder.imageCount(); imgindex++) {
                imgReadreder.jumpToImage(imgindex);
                m_re->m_imgs << imgReadreder.read();
            }
        } else {
            // QImage不应该多次赋值，所以换到这里来，修复style问题
            QImage img;
            LibUnionImage_NameSpace::loadStaticImageFromFile(path, img, errMsg);
            if (!img.isNull()) {
                m_re->m_imgs << img;
            }
        }
        tempExsitPaths << paths;
    }

    // 看图采用同步,因为只有一张图片，传入父指针
    DPrintPreviewDialog printDialog2(parent);
#if (DTK_VERSION_MAJOR > 5 || (DTK_VERSION_MAJOR >= 5 && DTK_VERSION_MINOR > 4) ||                                               \
     (DTK_VERSION_MAJOR >= 5 && DTK_VERSION_MINOR >= 4 && DTK_VERSION_PATCH >= 10))  // 5.4.4暂时没有合入
    // 增加运行时版本判断
    if (DApplication::runtimeDtkVersion() >= DTK_VERSION_CHECK(5, 4, 10, 0)) {
        if (!tempExsitPaths.isEmpty()) {
            // 直接传递为路径,不会有问题
            printDialog2.setDocName(QFileInfo(tempExsitPaths.at(0)).absoluteFilePath());
        }
    }
#endif

#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
    // 检查是否过滤 DTK 部分设置
    if (PermissionConfig::instance()->installFilterPrintDialog(&printDialog2)) {
        qInfo() << qPrintable("Enable breakPrintSpacingLimit, filter print spacing config.");
    }

    // 定制分支，水印功能不依赖DTK版本，更新打印水印设置
    if (PermissionConfig::instance()->hasPrintWaterMark()) {
        // 使用适配的水印配置
        auto data = PermissionConfig::instance()->adapterPrintWaterMarkData();

        DPrintPreviewSettingInfo *baseInfo = printDialog2.createDialogSettingInfo(DPrintPreviewWatermarkInfo::PS_Watermark);
        if (baseInfo) {
            DPrintPreviewWatermarkInfo *info = dynamic_cast<DPrintPreviewWatermarkInfo *>(baseInfo);
            if (info) {
                // 打印水印实现方式同阅读水印略有不同，调整参数以使得效果一致。
                info->opened = true;

                info->angle = static_cast<int>(data.rotation);
                info->transparency = static_cast<int>(data.opacity * 100);
                QFontMetrics fm(data.font);
                QSize textSize = fm.size(Qt::TextSingleLine, data.text);
                if (textSize.height() > 0) {
                    info->rowSpacing = qMax(0.0, (qreal(data.lineSpacing + textSize.height()) / textSize.height()) - 1.0);
                }
                if (textSize.width() > 0) {
                    info->columnSpacing = qMax(0.0, (qreal(data.spacing + textSize.width()) / textSize.width()) - 1.0);
                }
                info->layout = (data.layout == PermissionConfig::AdapterWaterMarkData::Center) ?
                                   DPrintPreviewWatermarkInfo::Center :
                                   DPrintPreviewWatermarkInfo::Tiled;
                info->currentWatermarkType = (data.type == PermissionConfig::AdapterWaterMarkData::Text) ?
                                                 DPrintPreviewWatermarkInfo::TextWatermark :
                                                 DPrintPreviewWatermarkInfo::ImageWatermark;
                info->textType = DPrintPreviewWatermarkInfo::Custom;
                info->customText = data.text;
                info->textColor = data.color;
                info->fontList.append(data.font.family());
                static const qreal sc_defaultFontSize = 65.0;
                // 字体使用缩放滑块处理 10%~200%, 默认字体大小为65
                info->size = int(data.font.pointSizeF() / sc_defaultFontSize * 100);

                printDialog2.updateDialogSettingInfo(info);
            } else {
                qWarning() << qPrintable("Can't convert DPrintPreviewDialog watermark info.") << baseInfo->type();
            }

            delete baseInfo;
        } else {
            qWarning() << qPrintable("Can't get DPrintPreviewDialog watermark info.");
        }

        // 在不使用打印插件时，需要手动设置打印插件选项显示且不可编辑，在设置后调用
        // WaterMarkFrame 和 WaterMarkContentFrame 是DTK窗口中水印设置控件的 objectName
        auto widgetList = printDialog2.findChildren<QWidget *>("WaterMarkFrame");
        widgetList.append(printDialog2.findChildren<QWidget *>("WaterMarkContentFrame"));
        for (auto wid : widgetList) {
            wid->setVisible(true);
            wid->setEnabled(false);
        }
    }
#endif

    connect(&printDialog2, SIGNAL(paintRequested(DPrinter *)), m_re, SLOT(paintRequestSync(DPrinter *)));

#ifndef USE_TEST
    int ret = printDialog2.exec();
#else
    printDialog2.show();
    int ret = QDialog::Accepted;
#endif

    if (QDialog::Accepted == ret) {
        if (!tempExsitPaths.isEmpty()) {
            PermissionConfig::instance()->triggerPrint(tempExsitPaths.first());
        }
    }

    m_re->m_paths.clear();
    m_re->m_imgs.clear();
}

RequestedSlot::RequestedSlot(QObject *parent)
{
    Q_UNUSED(parent)
}

RequestedSlot::~RequestedSlot() {}

void RequestedSlot::paintRequestSync(DPrinter *_printer)
{
    // 由于之前再度修改了打印的逻辑，导致了相同图片不在被显示，多余多页tiff来说不合理
    QPainter painter(_printer);
    int indexNum = 0;
    for (QImage img : m_imgs) {
        if (!img.isNull()) {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            QRect wRect = _printer->pageRect();
            // 修复bug98129，打印不完全问题，ratio应该是适应宽或者高，不应该直接适应宽
            qreal ratio = 0.0;
            qDebug() << wRect;
            ratio = wRect.width() * 1.0 / img.width();
            if (qreal(wRect.height() - img.height() * ratio) > 0) {
                painter.drawImage(
                    QRectF(0, abs(qreal(wRect.height() - img.height() * ratio)) / 2, wRect.width(), img.height() * ratio), img);
            } else {
                ratio = wRect.height() * 1.0 / img.height();
                painter.drawImage(QRectF(qreal(wRect.width() - img.width() * ratio) / 2, 0, img.width() * ratio, wRect.height()),
                                  img);
            }
        }
        indexNum++;
        if (indexNum != m_imgs.size()) {
            _printer->newPage();
        }
    }
    painter.end();
}
