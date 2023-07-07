// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
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

//暂时没有使用配置文件的快捷键，现在是根据代码中的快捷键
/*
static QAction *hookToolBarActionIcons(QToolBar *bar, QAction **pageSetupAction = nullptr)
{
    QAction *last_action = nullptr;

    for (QAction *action : bar->actions()) {
        const QString &text = action->text();

        if (text.isEmpty())
            continue;

        // 防止被lupdate扫描出来
        const char *context = "QPrintPreviewDialog";
        const char *print = "Print";

        const QMap<QString, QString> map {
            {QCoreApplication::translate(context, "Next page"), QStringLiteral("go-next")},
            {QCoreApplication::translate(context, "Previous page"), QStringLiteral("go-previous")},
            {QCoreApplication::translate(context, "First page"), QStringLiteral("go-first")},
            {QCoreApplication::translate(context, "Last page"), QStringLiteral("go-last")},
            {QCoreApplication::translate(context, "Fit width"), QStringLiteral("fit-width")},
            {QCoreApplication::translate(context, "Fit page"), QStringLiteral("fit-page")},
            {QCoreApplication::translate(context, "Zoom in"), QStringLiteral("zoom-in")},
            {QCoreApplication::translate(context, "Zoom out"), QStringLiteral("zoom-out")},
            {QCoreApplication::translate(context, "Portrait"), QStringLiteral("layout-portrait")},
            {QCoreApplication::translate(context, "Landscape"), QStringLiteral("layout-landscape")},
            {QCoreApplication::translate(context, "Show single page"), QStringLiteral("view-page-one")},
            {QCoreApplication::translate(context, "Show facing pages"), QStringLiteral("view-page-sided")},
            {QCoreApplication::translate(context, "Show overview of all pages"), QStringLiteral("view-page-multi")},
            {QCoreApplication::translate(context, print), QStringLiteral("print")},
            {QCoreApplication::translate(context, "Page setup"), QStringLiteral("page-setup")}
        };


        const QString &icon_name = map.value(action->text());

        if (icon_name.isEmpty())
            continue;

        if (pageSetupAction && icon_name == "page-setup") {
            *pageSetupAction = action;
        }

        QIcon icon(QStringLiteral(":/qt-project.org/dialogs/assets/images/qprintpreviewdialog/images/%1-24.svg").arg(icon_name));
//        action->setIcon(icon);
        last_action = action;
    }

    return last_action;
}
*/
void PrintHelper::showPrintDialog(const QStringList &paths, QWidget *parent)
{
    if (!PermissionConfig::instance()->isPrintable()) {
        return;
    }

    m_re->m_paths.clear();
    m_re->m_imgs.clear();

    m_re->m_paths = paths;
    QStringList tempExsitPaths;//保存存在的图片路径
    for (const QString &path : paths) {
        QString errMsg;
        QImageReader imgReadreder(path);
        if (imgReadreder.imageCount() > 1) {
            for (int imgindex = 0; imgindex < imgReadreder.imageCount(); imgindex++) {
                imgReadreder.jumpToImage(imgindex);
                m_re->m_imgs << imgReadreder.read();
            }
        } else {
            //QImage不应该多次赋值，所以换到这里来，修复style问题
            QImage img;
            LibUnionImage_NameSpace::loadStaticImageFromFile(path, img, errMsg);
            if (!img.isNull()) {
                m_re->m_imgs << img;
            }
        }
        tempExsitPaths << paths;
    }

    //看图采用同步,因为只有一张图片，传入父指针
    DPrintPreviewDialog printDialog2(parent);    
#if (DTK_VERSION_MAJOR > 5 \
    || (DTK_VERSION_MAJOR >=5 && DTK_VERSION_MINOR > 4) \
    || (DTK_VERSION_MAJOR >= 5 && DTK_VERSION_MINOR >= 4 && DTK_VERSION_PATCH >= 10))//5.4.4暂时没有合入
    //增加运行时版本判断
    if (DApplication::runtimeDtkVersion() >= DTK_VERSION_CHECK(5, 4, 10, 0)) {
        if (!tempExsitPaths.isEmpty()) {
            //直接传递为路径,不会有问题
            printDialog2.setDocName(QFileInfo(tempExsitPaths.at(0)).absoluteFilePath());
        }
    }
#endif

#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
    // 定制分支，水印功能不依赖DTK版本
    // 更新打印水印设置
    if (PermissionConfig::instance()->hasPrintWaterMark()) {
        auto data = PermissionConfig::instance()->printWaterMarkData();

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
                    info->rowSpacing = qreal(data.lineSpacing + textSize.height()) / textSize.height();
                }
                if (textSize.width() > 0) {
                    info->columnSpacing = qreal(data.spacing + textSize.width()) / textSize.width();
                }
                info->layout = data.layout == WaterMarkLayout::Center ? DPrintPreviewWatermarkInfo::Center : DPrintPreviewWatermarkInfo::Tiled;
                info->currentWatermarkType = (data.type == WaterMarkType::Text) ? DPrintPreviewWatermarkInfo::TextWatermark : DPrintPreviewWatermarkInfo::ImageWatermark;
                info->textType = DPrintPreviewWatermarkInfo::Custom;
                info->customText = data.text;
                info->textColor = data.color;
                info->fontList.append(data.font.family());
                static const float sc_defaultFontSize = 65.0f;
                // 字体使用缩放滑块处理 10%~200%, 默认字体大小为65
                info->size = int(data.font.pixelSize() / sc_defaultFontSize * 100);
                printDialog2.updateDialogSettingInfo(info);
            } else {
                qWarning() << qPrintable("Can't convert DPrintPreviewDialog watermark info.") << baseInfo->type();
            }

            delete baseInfo;
        } else {
            qWarning() << qPrintable("Can't get DPrintPreviewDialog watermark info.");
        }
    }
#endif

    connect(&printDialog2, SIGNAL(paintRequested(DPrinter *)),
            m_re, SLOT(paintRequestSync(DPrinter *)));

#ifndef USE_TEST
    int ret = printDialog2.exec();
#else
    int ret = printDialog2.show();
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

RequestedSlot::~RequestedSlot()
{

}

void RequestedSlot::paintRequestSync(DPrinter *_printer)
{
    //由于之前再度修改了打印的逻辑，导致了相同图片不在被显示，多余多页tiff来说不合理
    QPainter painter(_printer);
    int indexNum = 0;
    for (QImage img : m_imgs) {
        if (!img.isNull()) {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            QRect wRect  = _printer->pageRect();
            //修复bug98129，打印不完全问题，ratio应该是适应宽或者高，不应该直接适应宽
            qreal ratio = 0.0;
            qDebug() << wRect;
            ratio = wRect.width() * 1.0 / img.width();
            if (qreal(wRect.height() - img.height() * ratio) > 0) {
                painter.drawImage(QRectF(0, abs(qreal(wRect.height() - img.height() * ratio)) / 2,
                                         wRect.width(), img.height() * ratio), img);
            } else {
                ratio = wRect.height() * 1.0 / img.height();
                painter.drawImage(QRectF(qreal(wRect.width() - img.width() * ratio) / 2, 0,
                                         img.width() * ratio, wRect.height()), img);
            }


        }
        indexNum++;
        if (indexNum != m_imgs.size()) {
            _printer->newPage();

        }
    }
    painter.end();
}

