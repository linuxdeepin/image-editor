// SPDX-FileCopyrightText: 2022 - 2024 UnionTech Software Technology Co., Ltd.
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
#include <dtkwidget_config.h>

#include "imageengine.h"
#include "viewpanel/viewpanel.h"
#include "service/permissionconfig.h"
#include "service/commonservice.h"
#include "service/mtpfileproxy.h"
#include "unionimage/imageutils.h"
#include "unionimage/baseutils.h"

#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
#include <DWaterMarkHelper>
#endif

#define PLUGINTRANSPATH "/usr/share/libimageviewer/translations"
class ImageViewerPrivate
{
public:
    ImageViewerPrivate(imageViewerSpace::ImgViewerType imgViewerType,
                       const QString &savePath,
                       AbstractTopToolbar *customTopToolbar,
                       ImageViewer *parent);

private:
    ImageViewer *q_ptr;
    LibViewPanel *m_panel = nullptr;
    imageViewerSpace::ImgViewerType m_imgViewerType;

    Q_DECLARE_PUBLIC(ImageViewer)
    Q_DISABLE_COPY(ImageViewerPrivate)
};

ImageViewerPrivate::ImageViewerPrivate(imageViewerSpace::ImgViewerType imgViewerType,
                                       const QString &savePath,
                                       AbstractTopToolbar *customTopToolbar,
                                       ImageViewer *parent)
    : q_ptr(parent)
{
    qDebug() << "Initializing image viewer private, viewer type:" << imgViewerType << "save path:" << savePath;
    // 初始化检测wayland环境
    Libutils::base::initCheckWaylandEnv();

    // 在界面前初始化授权配置
    if (!qApp) {
        qWarning() << "Must init authorise config after QApplication initialized!";
    } else {
        PermissionConfig::instance()->initFromArguments(qApp->arguments());
    }

    QDir dir(PLUGINTRANSPATH);
    if (dir.exists()) {
        QDirIterator qmIt(PLUGINTRANSPATH, QStringList() << QString("*%1.qm").arg(QLocale::system().name()), QDir::Files);

        while (qmIt.hasNext()) {
            qmIt.next();
            QFileInfo finfo = qmIt.fileInfo();
            QTranslator *translator = new QTranslator(qApp);
            if (translator->load(finfo.baseName(), finfo.absolutePath())) {
                qApp->installTranslator(translator);
                qDebug() << "Installed translator for:" << finfo.baseName();
            }
        }

#if QT_VERSION_CHECK(6, 0, 0) < QT_VERSION
        QStringList parseLocalNameList = QLocale::system().name().split("_", Qt::SkipEmptyParts);
#else
        QStringList parseLocalNameList = QLocale::system().name().split("_", QString::SkipEmptyParts);
#endif
        if (parseLocalNameList.length() > 0) {
            QString translateFilename = QString("/libimageviewer_%2.qm").arg(parseLocalNameList.at(0));

            QString translatePath = PLUGINTRANSPATH + translateFilename;
            if (QFile::exists(translatePath)) {
                qDebug() << "Loading translation file:" << translatePath;
                auto translator = new QTranslator(qApp);
                translator->load(translatePath);
                qApp->installTranslator(translator);
            }
        }
    }

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
    qDebug() << "Image viewer panel initialized";

#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
    // 设置看图水印，目前仅在主要展示区域显示
    if (PermissionConfig::instance()->hasReadWaterMark()) {
        qDebug() << "Setting up watermark for image viewer";
        auto data = PermissionConfig::instance()->readWaterMarkData();
        DWaterMarkHelper::instance()->setData(data);
        DWaterMarkHelper::instance()->registerWidget(m_panel);

        // 仅权限控制图片单独展示
        QObject::connect(PermissionConfig::instance(),
                         &PermissionConfig::currentImagePathChanged,
                         q_ptr,
                         [this](const QString &, bool isTargetImage) {
                             if (!PermissionConfig::instance()->isValid()) {
                                 return;
                             }

                             DWaterMarkWidget *mark = m_panel->findChild<DWaterMarkWidget *>();
                             if (mark) {
                                 mark->setVisible(isTargetImage);
                                 qDebug() << "Watermark visibility changed to:" << isTargetImage;
                             }
                         });
    }
#endif
}

/**
   @brief 构造视图展示控件
   @note 由于兼容性问题，此处的 `savePath` 没有调整为引用传递
 */
ImageViewer::ImageViewer(imageViewerSpace::ImgViewerType imgViewerType,
                         QString savePath,
                         AbstractTopToolbar *customTopToolbar,
                         QWidget *parent)
    : DWidget(parent)
    , d_ptr(new ImageViewerPrivate(imgViewerType, savePath, customTopToolbar, this))
{
    qDebug() << "Image viewer constructed";
    Q_INIT_RESOURCE(icons);
}

ImageViewer::~ImageViewer()
{
    qDebug() << "Image viewer being destroyed";
    //析构时删除m_panel
    Q_D(ImageViewer);
    d->m_panel->deleteLater();
    d->m_panel = nullptr;
    d->q_ptr = nullptr;
}

bool ImageViewer::startChooseFileDialog()
{
    qDebug() << "Starting file choose dialog";
    Q_D(ImageViewer);
    return d->m_panel->startChooseFileDialog();
}

bool ImageViewer::startdragImage(const QStringList &paths, const QString &firstPath, bool isCustom, const QString &album)
{
    qInfo() << "Starting drag image operation with" << paths.size() << "images, first path:" << firstPath 
            << "is custom:" << isCustom << "album:" << album;
    Q_D(ImageViewer);
    d->m_panel->setIsCustomAlbum(isCustom, album);
    return d->m_panel->startdragImage(paths, firstPath);
}

bool ImageViewer::startdragImageWithUID(
    const QStringList &paths, const QString &firstPath, bool isCustom, const QString &album, int UID)
{
    qInfo() << "Starting drag image operation with UID:" << UID << "paths:" << paths.size() 
            << "first path:" << firstPath << "is custom:" << isCustom << "album:" << album;
    Q_D(ImageViewer);
    d->m_panel->setIsCustomAlbumWithUID(isCustom, album, UID);
    return d->m_panel->startdragImage(paths, firstPath);
}

void ImageViewer::startImgView(const QString &currentPath, const QStringList &paths)
{
    qInfo() << "Starting image view with current path:" << currentPath << "total paths:" << paths.size();
    Q_D(ImageViewer);

    QStringList realPaths = paths;
    QString realPath = currentPath;
    MtpFileProxy::instance()->checkAndCreateProxyFile(realPaths, realPath);

    //展示当前图片
    d->m_panel->loadImage(realPath, realPaths);

    //启动线程制作缩略图
    if (LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeLocal ||
        LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerTypeNull) {
        qDebug() << "Generating thumbnails for local image viewer";
        //首先生成当前图片的缓存
        ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), QStringList(realPath), 1);
        //看图制作全部缩略图
        ImageEngine::instance()->makeImgThumbnail(LibCommonService::instance()->getImgSavePath(), realPaths, realPaths.size());
    }
}

void ImageViewer::switchFullScreen()
{
    qDebug() << "Switching fullscreen mode";
    Q_D(ImageViewer);
    d->m_panel->toggleFullScreen();
}

void ImageViewer::startSlideShow(const QStringList &paths, const QString &firstPath)
{
    qInfo() << "Starting slideshow with" << paths.size() << "images, first path:" << firstPath;
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
    qDebug() << "Setting top bar visibility:" << visible;
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setTopBarVisible(visible);
    }
}

void ImageViewer::setBottomtoolbarVisible(bool visible)
{
    qDebug() << "Setting bottom toolbar visibility:" << visible;
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setBottomtoolbarVisible(visible);
    }
}

DIconButton *ImageViewer::getBottomtoolbarButton(imageViewerSpace::ButtonType type)
{
    qDebug() << "Getting bottom toolbar button of type:" << type;
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
        qDebug() << "Getting current path:" << path;
    }
    return path;
}

void ImageViewer::setViewPanelContextMenuItemVisible(imageViewerSpace::NormalMenuItemId id, bool visible)
{
    qDebug() << "Setting context menu item visibility, id:" << id << "visible:" << visible;
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setContextMenuItemVisible(id, visible);
    }
}

void ImageViewer::setBottomToolBarButtonAlawysNotVisible(imageViewerSpace::ButtonType id, bool notVisible)
{
    qDebug() << "Setting bottom toolbar button always not visible, id:" << id << "not visible:" << notVisible;
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->setBottomToolBarButtonAlawysNotVisible(id, notVisible);
    }
}

void ImageViewer::setCustomAlbumName(const QMap<QString, bool> map, bool isFav)
{
    qDebug() << "Setting custom album names, is favorite:" << isFav;
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->updateCustomAlbum(map, isFav);
    }
}

void ImageViewer::setCustomAlbumNameAndUID(const QMap<int, std::pair<QString, bool> > &map, bool isFav)
{
    qDebug() << "Setting custom album names with UID, is favorite:" << isFav;
    Q_D(ImageViewer);
    if (d->m_panel) {
        d->m_panel->updateCustomAlbumAndUID(map, isFav);
    }
}

void ImageViewer::setDropEnabled(bool enable)
{
    qDebug() << "Setting drop enabled:" << enable;
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
