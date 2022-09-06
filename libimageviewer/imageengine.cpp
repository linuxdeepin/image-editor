// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
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
#include "service/imagedataservice.h"
class ImageEnginePrivate
{
public:
    explicit ImageEnginePrivate(ImageEngine *parent = nullptr);
    ~ImageEnginePrivate();

public:
    ImageEngine *const q_ptr;
    LibImgOperate *m_worker = nullptr;
//    QString m_thumbnailSavePath;//缩略图保存路径
    Q_DECLARE_PUBLIC(ImageEngine)
};

ImageEnginePrivate::ImageEnginePrivate(ImageEngine *parent): q_ptr(parent)
{
    Q_Q(ImageEngine);

    QThread *workerThread = new QThread(q_ptr);
    m_worker = new LibImgOperate(workerThread);
    m_worker->moveToThread(workerThread);
    //一张缩略图制作完成，发送到主线程
    q->connect(m_worker, &LibImgOperate::sigOneImgReady, LibCommonService::instance(), &LibCommonService::slotSetImgInfoByPath);
    workerThread->start();
}

ImageEnginePrivate::~ImageEnginePrivate()
{

}

ImageEngine *ImageEngine::m_ImageEngine = nullptr;

ImageEngine *ImageEngine::instance(QObject *parent)
{
    Q_UNUSED(parent);
    if (!m_ImageEngine) {
        m_ImageEngine = new ImageEngine();
    }
    return m_ImageEngine;
}

ImageEngine::ImageEngine(QWidget *parent)
    : QObject(parent)
    , d_ptr(new ImageEnginePrivate(this))
{
//    Q_D(ImageEngine);
}

ImageEngine::~ImageEngine()
{

}

void ImageEngine::makeImgThumbnail(QString thumbnailSavePath, QStringList paths, int makeCount, bool remake)
{
    //执行子线程制作缩略图动作
//    QMetaObject::invokeMethod(d->m_worker, "slotMakeImgThumbnail"
//                              , Q_ARG(QString, thumbnailSavePath)
//                              , Q_ARG(QStringList, paths)
//                              , Q_ARG(int, makeCount)
//                              , Q_ARG(bool, remake)
//                             );
    Q_UNUSED(makeCount);
    if (paths.count() > 0) {
        LibImageDataService::instance()->readThumbnailByPaths(thumbnailSavePath, paths, remake);
    }
}
//判断是否图片格式
bool ImageEngine::isImage(const QString &path)
{
    bool bRet = false;
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    QMimeType mt1 = db.mimeTypeForFile(path, QMimeDatabase::MatchExtension);
    if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng") ||
            mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
        bRet = true;
    }
    return bRet;
}

bool ImageEngine::isRotatable(const QString &path)
{
    //BUG#93143
    //由于底层代码判断是否可旋转的依据是文件名后缀所代表的图片是否可保存
    //因此文件是否存在的判断添加在这一层
    QFileInfo info(path);
    if (!info.isFile() || !info.exists() || !info.isWritable()) {
        return false;
    } else {
        return LibUnionImage_NameSpace::isImageSupportRotate(path);
    }
}
//根据文件路径制作md5
QString ImageEngine::makeMD5(const QString &path)
{
    QFile file(path);
    QString  stHashValue;
    if (file.open(QIODevice::ReadOnly)) { //只读方式打开
        QCryptographicHash hash(QCryptographicHash::Md5);

        QByteArray buf = file.read(10 * 1024 * 1024); // 每次读取10M
        buf = buf.append(path.toUtf8());
        hash.addData(buf);  // 将数据添加到Hash中
        stHashValue.append(hash.result().toHex());
    }
    return stHashValue;
}
