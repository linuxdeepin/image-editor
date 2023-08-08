// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imageviewer.h"

#include <QDebug>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QDir>
#include <QCollator>
#include <QCommandLineParser>
#include <QDirIterator>
#include <QTranslator>
#include <DFileDialog>

#include "imageengine.h"
#include "viewpanel/viewpanel.h"
#include "service/commonservice.h"
#include "service/mtpfileproxy.h"
#include "unionimage/imageutils.h"
#include "unionimage/baseutils.h"

#define PLUGINTRANSPATH "/usr/share/libimageviewer/translations"
class ImageViewerPrivate
{
public:
    ImageViewerPrivate(imageViewerSpace::ImgViewerType imgViewerType, QString savePath, AbstractTopToolbar *customTopToolbar, ImageViewer *parent);
private:
    ImageViewer     *q_ptr;
    LibViewPanel       *m_panel = nullptr;
    imageViewerSpace::ImgViewerType   m_imgViewerType;
    Q_DECLARE_PUBLIC(ImageViewer)
};

ImageViewerPrivate::ImageViewerPrivate(imageViewerSpace::ImgViewerType imgViewerType, QString savePath, AbstractTopToolbar *customTopToolbar, ImageViewer *parent)
    : q_ptr(parent)
{
    QDir dir(PLUGINTRANSPATH);

    if (dir.exists()) {
        QDirIterator qmIt(PLUGINTRANSPATH, QStringList() << QString("*%1.qm").arg(QLocale::system().name()), QDir::Files);

        while (qmIt.hasNext()) {
            qmIt.next();
            QFileInfo finfo = qmIt.fileInfo();
            QTranslator *translator = new QTranslator(qApp);
            if (translator->load(finfo.baseName(), finfo.absolutePath())) {
                qApp->installTranslator(translator);
            }
        }

        QStringList parseLocalNameList = QLocale::system().name().split("_", QString::SkipEmptyParts);
        if (parseLocalNameList.length() > 0) {
            QString  translateFilename = QString("/libimageviewer_%2.qm")
                                         .arg(parseLocalNameList.at(0));

            QString translatePath = PLUGINTRANSPATH + translateFilename;
            if (QFile::exists(translatePath)) {
                qDebug() << "translatePath after feedback:" << translatePath;
                auto translator = new QTranslator(qApp);
                translator->load(translatePath);
                qApp->installTranslator(translator);
            }
        }
    }
//    }
    Q_Q(ImageViewer);
    m_imgViewerType = imgViewerType;
    //记录当前展示模式
    LibCommonService::instance()->setImgViewerType(imgViewerType);
    //记录缩略图保存路径
    LibCommonService::instance()->setImgSavePath(savePath);

    QVBoxLayout *layout = new QVBoxLayout(q);
    layout->setContentsMargins(0, 0, 0, 0);
    q->setLayout(layout);
    m_panel = new LibViewPanel(customTopToolbar, q);
    layout->addWidget(m_panel);
}

ImageViewer::ImageViewer(imageViewerSpace::ImgViewerType imgViewerType, QString savePath, AbstractTopToolbar *customTopToolbar, QWidget *parent)
    : DWidget(parent)
    , d_ptr(new ImageViewerPrivate(imgViewerType, savePath, customTopToolbar, this))
{
    Q_INIT_RESOURCE(icons);
}

ImageViewer::~ImageViewer()
{
    //析构时删除m_panel
    Q_D(ImageViewer);
    d->m_panel->deleteLater();
    d->m_panel = nullptr;
    d->q_ptr = nullptr;
}

bool ImageViewer::startChooseFileDialog()
{
    Q_D(ImageViewer);
    return d->m_panel->startChooseFileDialog();
}

bool ImageViewer::startdragImage(const QStringList &paths, const QString &firstPath, bool isCustom, const QString &album)
{
    Q_D(ImageViewer);
    d->m_panel->setIsCustomAlbum(isCustom, album);
    return d->m_panel->startdragImage(paths, firstPath);
}

bool ImageViewer::startdragImageWithUID(const QStringList &paths, const QString &firstPath, bool isCustom, const QString &album, int UID)
{
    Q_D(ImageViewer);
    d->m_panel->setIsCustomAlbumWithUID(isCustom, album, UID);
    return d->m_panel->startdragImage(paths, firstPath);
}

void ImageViewer::startImgView(QString currentPath, QStringList paths)
{
    Q_D(ImageViewer);

    QStringList realPaths = paths;
    QString realPath = currentPath;
    MtpFileProxy::instance()->checkAndCreateProxyFile(realPaths, realPath);

    //展示当前图片
    d->m_panel->loadImage(realPath, realPaths);

    //启动线程制作缩略图
    if (LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeLocal ||
            LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeNull) {
        //首先生成当前图片的缓存
        ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), QStringList(realPath), 1);
        //看图制作全部缩略图
        ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), realPaths, realPaths.size());
    }
}

void ImageViewer::switchFullScreen()
{
    Q_D(ImageViewer);
    d->m_panel->toggleFullScreen();
}

void ImageViewer::startSlideShow(const QStringList &paths, const QString &firstPath)
{
    Q_D(ImageViewer);

    ViewInfo info;
    info.fullScreen = window()->isFullScreen();
    info.lastPanel = this;
    info.path = firstPath;
    info.paths = paths;
    MtpFileProxy::instance()->checkAndCreateProxyFile(info.paths, info.path);

    info.viewMainWindowID = 0;
    d->m_panel->startSlideShow(info);
}

void ImageViewer::setTopBarVisible(bool visible)
{
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setTopBarVisible(visible);
    }
}

void ImageViewer::setBottomtoolbarVisible(bool visible)
{
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setBottomtoolbarVisible(visible);
    }
}

DIconButton *ImageViewer::getBottomtoolbarButton(imageViewerSpace::ButtonType type)
{
    DIconButton *button = nullptr;
    Q_D(ImageViewer);
    if (d->m_panel) {
        button = d->m_panel->getBottomtoolbarButton(type);
    }
    return button;
}

QString ImageViewer::getCurrentPath()
{
    Q_D(ImageViewer);
    QString path;
    if (d->m_panel) {
        path = d->m_panel->getCurrentPath();
    }
    return path;
}

void ImageViewer::setViewPanelContextMenuItemVisible(imageViewerSpace::NormalMenuItemId id, bool visible)
{
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setContextMenuItemVisible(id, visible);
    }
}

void ImageViewer::setBottomToolBarButtonAlawysNotVisible(imageViewerSpace::ButtonType id, bool notVisible)
{
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setBottomToolBarButtonAlawysNotVisible(id, notVisible);
    }
}

void ImageViewer::setCustomAlbumName(const QMap<QString, bool> map, bool isFav)
{
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->updateCustomAlbum(map, isFav);
    }
}

void ImageViewer::setCustomAlbumNameAndUID(const QMap<int, std::pair<QString, bool> > &map, bool isFav)
{
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->updateCustomAlbumAndUID(map, isFav);
    }
}

void ImageViewer::setDropEnabled(bool enable)
{
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setAcceptDrops(enable);
    }
}

void ImageViewer::resizeEvent(QResizeEvent *e)
{
    DWidget::resizeEvent(e);
}

void ImageViewer::showEvent(QShowEvent *e)
{
    return DWidget::showEvent(e);
}
