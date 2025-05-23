// SPDX-FileCopyrightText: 2022 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imageengine.h"

#include <QDebug>
#include <QThread>
#include <QStandardPaths>
#include <QDir>
#include <QMimeDatabase>
#include <QUrl>
#include <QCryptographicHash>

#include "service/commonservice.h"
#include "unionimage/imgoperate.h"
#include "unionimage/unionimage.h"
#include "service/permissionconfig.h"
#include "service/imagedataservice.h"

class ImageEnginePrivate
{
public:
    explicit ImageEnginePrivate(ImageEngine *parent = nullptr);
    ~ImageEnginePrivate();

public:
    ImageEngine *const q_ptr;
    LibImgOperate *m_worker = nullptr;

    Q_DECLARE_PUBLIC(ImageEngine)
    Q_DISABLE_COPY(ImageEnginePrivate)
};

ImageEnginePrivate::ImageEnginePrivate(ImageEngine *parent)
    : q_ptr(parent)
{
    qDebug() << "Initializing image engine private";
    Q_Q(ImageEngine);
    // 关联授权操作通知信号
    QObject::connect(PermissionConfig::instance(), &PermissionConfig::authoriseNotify, q, &ImageEngine::sigAuthoriseNotify);

    QThread *workerThread = new QThread(q_ptr);
    m_worker = new LibImgOperate(workerThread);
    m_worker->moveToThread(workerThread);
    //一张缩略图制作完成，发送到主线程
    q->connect(m_worker, &LibImgOperate::sigOneImgReady, LibCommonService::instance(), &LibCommonService::slotSetImgInfoByPath);
    workerThread->start();
    qDebug() << "Image engine worker thread started";
}

ImageEnginePrivate::~ImageEnginePrivate() 
{
    qDebug() << "Destroying image engine private";
}

ImageEngine *ImageEngine::m_ImageEngine = nullptr;

ImageEngine *ImageEngine::instance(QObject *parent)
{
    Q_UNUSED(parent);
    if (!m_ImageEngine) {
        qDebug() << "Creating new image engine instance";
        m_ImageEngine = new ImageEngine();
    }
    return m_ImageEngine;
}

ImageEngine::ImageEngine(QWidget *parent)
    : QObject(parent)
    , d_ptr(new ImageEnginePrivate(this))
{
    qDebug() << "Image engine constructed";
}

ImageEngine::~ImageEngine() 
{
    qDebug() << "Image engine destroyed";
}

/**
   @brief 构造传入图像 `paths` 的缩略图到路径 `thumbnailSavePath` , `remake` 标识是否重新创建
   @note 由于兼容性问题，此处的 `thumbnailSavePath` `paths` 没有调整为引用传递
 */
void ImageEngine::makeImgThumbnail(QString thumbnailSavePath, QStringList paths, int makeCount, bool remake)
{
    qDebug() << "Making thumbnails for" << paths.size() << "images, save path:" << thumbnailSavePath << "remake:" << remake;
    //执行子线程制作缩略图动作
    Q_UNUSED(makeCount);
    if (paths.count() > 0) {
        LibImageDataService::instance()->readThumbnailByPaths(thumbnailSavePath, paths, remake);
    } else {
        qWarning() << "No paths provided for thumbnail generation";
    }
}

//判断是否图片格式
bool ImageEngine::isImage(const QString &path)
{
    qDebug() << "Checking if file is image:" << path;
    bool bRet = false;
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    QMimeType mt1 = db.mimeTypeForFile(path, QMimeDatabase::MatchExtension);
    if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng") || mt1.name().startsWith("image/") ||
        mt1.name().startsWith("video/x-mng")) {
        bRet = true;
    }
    qDebug() << "File" << path << "is" << (bRet ? "an image" : "not an image");
    return bRet;
}

bool ImageEngine::isRotatable(const QString &path)
{
    qDebug() << "Checking if image is rotatable:" << path;
    // BUG#93143
    //由于底层代码判断是否可旋转的依据是文件名后缀所代表的图片是否可保存
    //因此文件是否存在的判断添加在这一层
    QFileInfo info(path);
    if (!info.isFile() || !info.exists() || !info.isWritable()) {
        qWarning() << "File cannot be rotated - not a file, doesn't exist, or not writable:" << path;
        return false;
    } else {
        bool rotatable = LibUnionImage_NameSpace::isImageSupportRotate(path);
        qDebug() << "Image rotation support:" << rotatable;
        return rotatable;
    }
}

//根据文件路径制作md5
QString ImageEngine::makeMD5(const QString &path)
{
    qDebug() << "Generating MD5 for file:" << path;
    QFile file(path);
    QString stHashValue;
    if (file.open(QIODevice::ReadOnly)) {  //只读方式打开
        QCryptographicHash hash(QCryptographicHash::Md5);

        QByteArray buf = file.read(10 * 1024 * 1024);  // 每次读取10M
        buf = buf.append(path.toUtf8());
        hash.addData(buf);  // 将数据添加到Hash中
        stHashValue.append(hash.result().toHex());
        qDebug() << "MD5 generated successfully";
    } else {
        qWarning() << "Failed to open file for MD5 generation:" << path;
    }
    return stHashValue;
}
