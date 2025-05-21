// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginbaseutils.h"
#include "unionimage.h"
#include "image-viewer_global.h"
#include "service/commonservice.h"

#include <stdio.h>
#include <fcntl.h>
#include <fstream>
#include <linux/fs.h>

#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFontMetrics>
#include <QFileInfo>
#include <QImage>
#include <QMimeData>
#include <QProcess>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QTextStream>
#include <QtMath>
#include <QDirIterator>
#include <QImageReader>
#include <QMimeDatabase>

#include <DApplication>
#include <DDesktopServices>

DWIDGET_USE_NAMESPACE

namespace pluginUtils {

namespace base {

const QString DATETIME_FORMAT_NORMAL = "yyyy.MM.dd";
const QString DATETIME_FORMAT_EXIF = "yyyy:MM:dd HH:mm:ss";

//int stringHeight(const QFont &f, const QString &str)
//{
//    QFontMetrics fm(f);
//    return fm.boundingRect(str).height();
//}
//
//QDateTime stringToDateTime(const QString &time)
//{
//    QDateTime dt = QDateTime::fromString(time, DATETIME_FORMAT_EXIF);
//    if (! dt.isValid()) {
//        dt = QDateTime::fromString(time, DATETIME_FORMAT_NORMAL);
//    }
//    return dt;
//}
//
//void showInFileManager(const QString &path)
//{
//    if (path.isEmpty() || !QFile::exists(path)) {
//        return;
//    }
//    QString m_Path = static_cast<QString>(path);
//
//    QStringList spc {"#", "&", "@", "!", "?"};
//    for (QString c : spc) {
//        m_Path.replace(c,  QUrl::toPercentEncoding(c));
//    }
//    QUrl url = QUrl::fromUserInput(/*"\"" + */m_Path/* + "\""*/);
//    url.setPath(m_Path, QUrl::TolerantMode);
//    Dtk::Widget::DDesktopServices::showFileItem(url);
//}

//void copyOneImageToClipboard(const QString &path)
//{
//    QImage img(path);
//    if (img.isNull())
//        return;
////    Q_ASSERT(!img.isNull());
//    QClipboard *cb = QApplication::clipboard();
//    cb->setImage(img, QClipboard::Clipboard);
//}

//void copyImageToClipboard(const QStringList &paths)
//{
//    //  Get clipboard
//    QClipboard *cb = qApp->clipboard();
//
//    // Ownership of the new data is transferred to the clipboard.
//    QMimeData *newMimeData = new QMimeData();
//    QByteArray gnomeFormat = QByteArray("copy\n");
//    QString text;
//    QList<QUrl> dataUrls;
//    for (QString path : paths) {
//        if (!path.isEmpty())
//            text += path + '\n';
//        dataUrls << QUrl::fromLocalFile(path);
//        gnomeFormat.append(QUrl::fromLocalFile(path).toEncoded()).append("\n");
//    }
//
//    newMimeData->setText(text.endsWith('\n') ? text.left(text.length() - 1) : text);
//    newMimeData->setUrls(dataUrls);
//    gnomeFormat.remove(gnomeFormat.length() - 1, 1);
//    newMimeData->setData("x-special/gnome-copied-files", gnomeFormat);
//
//    // Copy Image Date
////    QImage img(paths.first());
////    Q_ASSERT(!img.isNull());
////    newMimeData->setImageData(img);
//
//    // Set the mimedata
//    cb->setMimeData(newMimeData, QClipboard::Clipboard);
//}

//QString getFileContent(const QString &file)
//{
//    QFile f(file);
//    QString fileContent = "";
//    if (f.open(QFile::ReadOnly)) {
//        fileContent = QLatin1String(f.readAll());
//        f.close();
//    }
//    return fileContent;
//}
//
//QString SpliteText(const QString &text, const QFont &font, int nLabelSize)
//{
////LMH0424，之前是递归，现在改了算法，判断换行
//    QFontMetrics fm(font);
//    double dobuleTextSize = fm.horizontalAdvance(text);
//    double dobuleLabelSize = nLabelSize;
//    if (dobuleTextSize > dobuleLabelSize && dobuleLabelSize > 0 && dobuleTextSize < 10000) {
//        double splitCount = dobuleTextSize / dobuleLabelSize;
//        int nCount = int(splitCount + 1);
//        QString textSplite;
//        QString textTotal = text;
//        for (int index = 0; index < nCount; ++index) {
//            int nPos = 0;
//            long nOffset = 0;
//            for (int i = 0; i < text.size(); i++) {
//                nOffset += fm.width(text.at(i));
//                if (nOffset >= nLabelSize) {
//                    nPos = i;
//                    break;
//                }
//            }
//            nPos = (nPos - 1 < 0) ? 0 : nPos - 1;
//            QString qstrLeftData;
//            if (nCount - 1 == index) {
//                qstrLeftData = textTotal;
//                textSplite += qstrLeftData;
//            } else {
//                qstrLeftData = textTotal.left(nPos);
//                textSplite += qstrLeftData + "\n";
//            }
//            textTotal = textTotal.mid(nPos);
//        }
//        return textSplite;
//    } else {
//        return text;
//    }
//}

//QString hash(const QString &str)
//{
//    return QString(QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex());
//}

bool checkMimeData(const QMimeData *mimeData)
{
    qDebug() << "Checking mime data for drag and drop";
    if (!mimeData->hasUrls()) {
        qDebug() << "No URLs found in mime data";
        return false;
    }
    QList<QUrl> urlList = mimeData->urls();
    if (1 > urlList.size()) {
        qDebug() << "Empty URL list in mime data";
        return false;
    }

    bool result = false;

    //遍历URL，只要存在图片就允许拖入
    for (QUrl url : urlList) {
        //判断图片更新
        QString path = url.toLocalFile();
        if (path.isEmpty()) {
            path = url.path();
        }
        qDebug() << "Processing URL path:" << path;
        QFileInfo fileinfo(path);
        if (fileinfo.isDir()) {
            if (LibCommonService::instance()->getImgViewerType() == imageViewerSpace::ImgViewerType::ImgViewerTypeAlbum) { //相册模式的时候额外允许文件夹拖入
                qDebug() << "Album mode: Directory drag allowed";
                result = true;
                break;
            } else {
                qDebug() << "Non-album mode: Directory drag not allowed";
                continue;
            }
        } else {
            QFileInfo info(path);
            QMimeDatabase db;
            QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
            QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
            QString str = info.suffix().toLower();
            qDebug() << "File suffix:" << str << "MIME type (content):" << mt.name() << "MIME type (extension):" << mt1.name();
            
            if (str.isEmpty()) {
                if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")) {
                    if (supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                        qDebug() << "Empty suffix but supported format found";
                        result = true;
                        break;
                    } else if (str.isEmpty()) {
                        qDebug() << "Empty suffix but valid image MIME type";
                        result = true;
                        break;
                    }
                }
            } else {
                if (mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
                    qDebug() << "Valid image format with extension";
                    result = true;
                    break;
                }
            }
            continue;
        }
    }

    qDebug() << "Mime data check result:" << result;
    return result;
}


//QPixmap renderSVG(const QString &filePath, const QSize &size)
//{
//    QImageReader reader;
//    QPixmap pixmap;

//    reader.setFileName(filePath);

//    if (reader.canRead() && reader.imageCount() > 0) {
//        const qreal ratio = dApp->getDAppNew()->devicePixelRatio();
//        reader.setScaledSize(size * ratio);
//        pixmap = QPixmap::fromImage(reader.read());
//        pixmap.setDevicePixelRatio(ratio);
//    } else {
//        pixmap.load(filePath);
//    }

//    return pixmap;
//}

QString mkMutiDir(const QString &path)   //创建多级目录
{
    qDebug() << "Creating directory structure for:" << path;
    QDir dir(path);
    if (dir.exists(path)) {
        qDebug() << "Directory already exists:" << path;
        return path;
    }
    QString parentDir = mkMutiDir(path.mid(0, path.lastIndexOf('/')));
    QString dirname = path.mid(path.lastIndexOf('/') + 1);
    QDir parentPath(parentDir);
    if (!dirname.isEmpty()) {
        qDebug() << "Creating subdirectory:" << dirname << "in" << parentDir;
        parentPath.mkpath(dirname);
    }
    return parentDir + "/" + dirname;
}

bool imageSupportRead(const QString &path)
{
    qDebug() << "Checking if image format is supported for:" << path;
    const QString suffix = QFileInfo(path).suffix();
    // take them here for good.
    QStringList errorList;
    errorList << "X3F";
    if (errorList.indexOf(suffix.toUpper()) != -1) {
        qWarning() << "Unsupported format:" << suffix;
        return false;
    }
    bool supported = LibUnionImage_NameSpace::unionImageSupportFormat().contains(suffix.toUpper());
    qDebug() << "Format support check result:" << supported;
    return supported;
}

const QFileInfoList getImagesInfo(const QString &dir, bool recursive)
{
    qDebug() << "Getting image information from directory:" << dir << "recursive:" << recursive;
    QFileInfoList infos;

    if (! recursive) {
        qDebug() << "Non-recursive directory scan";
        auto nsl = QDir(dir).entryInfoList(QDir::Files);
        for (QFileInfo info : nsl) {
            if (imageSupportRead(info.absoluteFilePath())) {
                infos << info;
            }
        }
        qDebug() << "Found" << infos.size() << "images in directory";
        return infos;
    }

    qDebug() << "Recursive directory scan";
    QDirIterator dirIterator(dir,
                             QDir::Files,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        if (imageSupportRead(dirIterator.fileInfo().absoluteFilePath())) {
            infos << dirIterator.fileInfo();
        }
    }

    qDebug() << "Found" << infos.size() << "images in directory and subdirectories";
    return infos;
}

QStringList supportedImageFormats()
{
    qDebug() << "Getting list of supported image formats";
    QStringList formats = LibUnionImage_NameSpace::unionImageSupportFormat();
    qDebug() << "Supported formats:" << formats;
    return formats;
}

}  // namespace base

}  // namespace utils
