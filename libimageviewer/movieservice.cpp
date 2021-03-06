/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "movieservice.h"
#include <QMetaType>
#include <QDirIterator>
#include <memory>
#include <deque>
#include <MediaInfoDLL/MediaInfoDLL.h>
#include <QProcess>
#include <QtDebug>
#include <QJsonDocument>
#include "service/gstreamervideothumbnailer.h"

#include <iostream>

#define SEEK_TIME "00:00:01"

MovieService *MovieService::m_movieService = nullptr;
std::once_flag MovieService::instanceFlag;
static const std::map<QString, int> audioChannelMap = {
    { "mono",           1}, { "stereo",         2},
    { "2.1",            3}, { "3.0",            3},
    { "3.0(back)",      3}, { "4.0",            4},
    { "quad",           4}, { "quad(side)",     4},
    { "3.1",            4}, { "5.0",            5},
    { "5.0(side)",      5}, { "4.1",            5},
    { "5.1",            6}, { "5.1(side)",      6},
    { "6.0",            6}, { "6.0(front)",     6},
    { "hexagonal",      6}, { "6.1",            7},
    { "6.1(back)",      7}, { "6.1(front)",     7},
    { "7.0",            7}, { "7.0(front)",     7},
    { "7.1",            8}, { "7.1(wide)",      8},
    { "7.1(wide-side)", 8}, { "octagonal",      8},
    { "hexadecagonal", 16}, { "downmix",        2},
    { "22.2",          24}
};

MovieService *MovieService::instance(QObject *parent)
{
    Q_UNUSED(parent)
    //??????????????????
    std::call_once(instanceFlag, [&]() {
        m_movieService = new MovieService;
        initGstreamerVideoThumbnailer();
    });
    return m_movieService;
}

MovieService::MovieService(QObject *parent)
    : QObject(parent)
{
    //??????ffmpeg????????????
    if (checkCommandExist("ffmpeg")) {
        resolutionPattern = "[0-9]+x[0-9]+";
        codeRatePattern = "[0-9]+\\skb/s";
        fpsPattern = "[0-9]+\\sfps";
        m_ffmpegExist = true;
    }

    //??????ffmpegthumbnailer????????????
    if (checkCommandExist("ffmpegthumbnailer")) {
        m_ffmpegthumbnailerExist = true;
    }
}

bool MovieService::checkCommandExist(const QString &command)
{
    try {
        QProcess bash;
        bash.start("bash");
        bash.waitForStarted();
        bash.write(("command -v " + command).toUtf8());
        bash.closeWriteChannel();
        if (!bash.waitForFinished()) {
            qWarning() << bash.errorString();
            return false;
        }
        auto output = bash.readAllStandardOutput();
        if (output.isEmpty()) {
            return false;
        } else {
            return true;
        }
    } catch (std::logic_error &e) {
        qWarning() << e.what();
        return false;
    }
}

MovieInfo MovieService::getMovieInfo(const QUrl &url)
{
    MovieInfo result;

    m_bufferMutex.lock();
    auto iter = std::find_if(m_movieInfoBuffer.begin(), m_movieInfoBuffer.end(), [url](const std::pair<QUrl, MovieInfo> &data) {
        return data.first == url;
    });
    if (iter != m_movieInfoBuffer.end()) {
        m_bufferMutex.unlock();
        return iter->second;
    }
    m_bufferMutex.unlock();

    if (url.isLocalFile()) {
        QFileInfo fi(url.toLocalFile());
        if (fi.exists() && fi.permission(QFile::Permission::ReadOwner)) { //?????????????????????????????????
            result = parseFromFile(fi);
        }
    }

    m_bufferMutex.lock();
    m_movieInfoBuffer.push_back(std::make_pair(url, result));
    if (m_movieInfoBuffer.size() > 30) {
        m_movieInfoBuffer.pop_front();
    }
    m_bufferMutex.unlock();

    return result;
}

static QString removeBrackets(const QString &str)
{
    QString result;
    if (str.isEmpty()) {
        return result;
    }

    std::vector<std::pair<int, int>> indexes;
    std::deque<int> stackIndex;

    for (int i = 0; i != str.size(); ++i) {
        if (str[i] == '(') {
            stackIndex.push_back(i);
        } else if (str[i] == ')') {
            if (stackIndex.size() == 1) {
                indexes.push_back({stackIndex[0], i});
            }
            stackIndex.pop_back();
        } else {
            continue;
        }
    }

    result = str;
    for (int i = static_cast<int>(indexes.size() - 1); i >= 0; --i) {
        auto data = indexes[static_cast<size_t>(i)];
        result = result.remove(data.first, data.second - data.first + 1);
    }

    return result;
}

static QString searchLineFromKeyString(const std::string &key, const QString &targetStr)
{
    //???????????????
    QTextStream dataStream(targetStr.toUtf8(), QIODevice::ReadOnly);
    QString infoString;

    //??????
    while (1) {
        auto currentLine = dataStream.readLine();
        if (currentLine.isEmpty()) {
            break;
        }

        auto index = currentLine.toStdString().find(key);
        if (index != std::string::npos) {
            infoString = currentLine.right(currentLine.size() - static_cast<int>(index + key.size()));
            infoString = removeBrackets(infoString);
            break;
        }
    }

    return infoString;
}

MovieInfo MovieService::parseFromFile(const QFileInfo &fi)
{
    auto info = getMovieInfo_mediainfo(fi);
    if (!info.valid) {
        if (m_ffmpegExist) {
            info = getMovieInfo_ffmpeg(fi);
        } else {
            info.filePath = fi.absoluteFilePath();
            info.fileType = fi.suffix().toLower();
            info.fileSize = fi.size();
            info.valid = true;
        }
    }

    return info;
}

QImage MovieService::getMovieCover(const QUrl &url, const QString &savePath)
{
    auto image = getMovieCover_gstreamer(url);
    if (image.isNull() && m_ffmpegthumbnailerExist) {
        image = getMovieCover_ffmpegthumbnailer(url, savePath);
    }
    return image;
}

QImage MovieService::getMovieCover_ffmpegthumbnailer(const QUrl &url, const QString &bufferPath)
{
    QImage image;
    if (!m_ffmpegthumbnailerExist) {
        return image;
    }

    QString path = url.toLocalFile();
    QFileInfo info(path);

    //QString savePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + info.fileName() + ".png");
    QString savePath(bufferPath + info.fileName() + ".png");

    QByteArray output;
    try {
        QProcess ffmpegthumbnailer;
        QStringList cmds{"-i", path, "-o", savePath};
        ffmpegthumbnailer.start("ffmpegthumbnailer", cmds, QIODevice::ReadOnly);
        if (!ffmpegthumbnailer.waitForFinished()) {
            qWarning() << ffmpegthumbnailer.errorString();
            return image;
        }
    } catch (std::logic_error &e) {
        qWarning() << e.what();
        return image;
    }

    image = QImage(savePath);

    QFile::remove(savePath);

    return image;
}

QImage MovieService::getMovieCover_gstreamer(const QUrl &url)
{
    return runGstreamerVideoThumbnailer(url);
}

MovieInfo MovieService::getMovieInfo_ffmpeg(const QFileInfo &fi)
{
    struct MovieInfo mi;

    if (!m_ffmpegExist) {
        mi.valid = false;
        return mi;
    }

    //?????????????????????ffmpeg?????????
    auto filePath = fi.absoluteFilePath();
    QByteArray output;
    try {
        QProcess ffmpeg;
        QStringList cmds{"-i", filePath, "-hide_banner"};
        ffmpeg.start("ffmpeg", cmds, QIODevice::ReadOnly);
        if (!ffmpeg.waitForFinished()) {
            qWarning() << ffmpeg.errorString();
            return mi;
        }
        output = ffmpeg.readAllStandardError(); //ffmpeg??????????????????????????????????????????????????????????????????????????????????????????
    } catch (std::logic_error &e) {
        qWarning() << e.what();
        return mi;
    }

    //??????????????????????????????output???

    //1.????????????
    QString ffmpegOut = QString::fromUtf8(output);
    if (ffmpegOut.endsWith("Invalid data found when processing input\n") ||
            ffmpegOut.endsWith("Permission denied\n")) {
        return mi;
    }

    //2.????????????
    mi.valid = true;

    //2.1.????????????
    mi.filePath = filePath;
    mi.fileType = fi.suffix().toLower();
    mi.fileSize = fi.size();

    //2.2.???????????????
    auto videoInfoString = searchLineFromKeyString("Video: ", ffmpegOut);
    if (!videoInfoString.isEmpty()) {
        auto videoStreamInfo = videoInfoString.split(", ");

        //????????????
        mi.vCodecID = videoStreamInfo[0].split(" ")[0];

        //?????????????????????
        QRegExp resolutionExp(resolutionPattern);
        if (resolutionExp.indexIn(videoInfoString) > 0) {
            mi.resolution = resolutionExp.cap(0);

            auto videoSize = mi.resolution.split("x");
            int size_w = videoSize[0].toInt();
            int size_h = videoSize[1].toInt();
            mi.proportion = static_cast<double>(size_w) / size_h;
        } else {
            mi.resolution = "-";
            mi.proportion = -1;
        }

        //??????
        QRegExp codeRateExp(codeRatePattern);
        if (codeRateExp.indexIn(videoInfoString) > 0) {
            auto codeRate = codeRateExp.cap(0);
            mi.vCodeRate = codeRate.split(" ")[0].toInt();
        } else {
            mi.vCodeRate = 0;
        }

        //??????
        QRegExp fpsExp(fpsPattern);
        if (fpsExp.indexIn(videoInfoString) > 0) {
            auto fps = fpsExp.cap(0);
            mi.fps = fps.split(" ")[0].toInt();
        } else {
            mi.fps = 0;
        }
    } else {
        mi.vCodecID = "-";
        mi.resolution = "-";
        mi.proportion = -1;
        mi.vCodeRate = 0;
        mi.fps = 0;
    }

    //2.3.????????????
    auto timeInfoString = searchLineFromKeyString("Duration: ", ffmpegOut);
    if (!timeInfoString.isEmpty()) {
        mi.duration = timeInfoString.split(", ")[0].split(".")[0];
        if (mi.duration == "N/A") {
            mi.duration = "-";
        }
    }

    //2.4.????????????
    auto audioInfoString = searchLineFromKeyString("Audio: ", ffmpegOut);
    if (!audioInfoString.isEmpty()) {
        auto audioStreamInfo = audioInfoString.split(", ");
        mi.aCodeID = audioStreamInfo[0].split(" ")[0];
        mi.sampling = audioStreamInfo[1].split(" ")[0].toInt();
        mi.channels = audioChannelMap.at(audioStreamInfo[2]);
        mi.aDigit = audioStreamInfo[3];

        if (audioStreamInfo.size() > 4) {
            mi.aCodeRate = audioStreamInfo[4].split(" ")[0].toInt();
        } else {
            mi.aCodeRate = 0;
        }
    } else {
        mi.aCodeID = "-";
        mi.sampling = 0;
        mi.channels = 0;
        mi.aDigit = "-";
        mi.aCodeRate = 0;
    }

    //????????????????????????
    return mi;
}

std::vector<std::pair<QString, QString>> searchGroupFromKey(const QString &key, const QStringList &datas)
{
    std::vector<std::pair<QString, QString>> result;

    int i = 0;

    //??????????????????
    for (; i != datas.size(); ++i) {
        if (datas[i] == key) {
            ++i;
            break;
        }
    }

    //????????????
    for (; i != datas.size(); ++i) {
        if (datas[i].isEmpty()) {
            break;
        }

        auto tokens = datas[i].split(",");
        if (tokens.size() < 2) { //????????????
            continue;
        }
        result.push_back(std::make_pair(tokens[0], tokens[1]));
    }

    return result;
}

template <typename T>
void checkIfNotDoInsert(QJsonObject *jsonObj, const QString &jsonKey, T data, T value)
{
    if (data != value) {
        jsonObj->insert(jsonKey, data);
    }
}

QJsonObject MovieService::getMovieInfoByJson(const QUrl &url)
{
    QJsonObject jsonRet;

    auto info = getMovieInfo(url);
    if (info.valid) {
        QJsonObject jsonObjBase;
        checkIfNotDoInsert(&jsonObjBase, "FilePath", info.filePath, QString("-"));
        checkIfNotDoInsert(&jsonObjBase, "FileType", info.fileType, QString("-"));
        checkIfNotDoInsert(&jsonObjBase, "Resolution", info.resolution, QString("-"));
        checkIfNotDoInsert(&jsonObjBase, "Creation", info.creation.toString(Qt::ISODate), QString());
        checkIfNotDoInsert(&jsonObjBase, "FileSize", info.fileSize, 0ll);
        checkIfNotDoInsert(&jsonObjBase, "Duration", info.duration, QString("-"));

        QJsonObject jsonObjVideo;
        checkIfNotDoInsert(&jsonObjVideo, "CodecID", info.vCodecID, QString("-"));
        checkIfNotDoInsert(&jsonObjVideo, "CodeRate", info.vCodeRate, 0ll);
        checkIfNotDoInsert(&jsonObjVideo, "Fps", info.fps, 0);
        checkIfNotDoInsert(&jsonObjVideo, "Proportion", info.proportion, -1.0);

        QJsonObject jsonObjAudio;
        checkIfNotDoInsert(&jsonObjAudio, "CodecID", info.aCodeID, QString("-"));
        checkIfNotDoInsert(&jsonObjAudio, "CodeRate", info.aCodeRate, 0ll);
        checkIfNotDoInsert(&jsonObjAudio, "ChannelCount", info.channels, 0);
        checkIfNotDoInsert(&jsonObjAudio, "Sampling", info.sampling, 0);

        jsonRet.insert("Base", jsonObjBase);
        jsonRet.insert("Video", jsonObjVideo);
        jsonRet.insert("Audio", jsonObjAudio);

        checkIfNotDoInsert(&jsonRet, "Base", jsonObjBase, QJsonObject());
        checkIfNotDoInsert(&jsonRet, "Video", jsonObjVideo, QJsonObject());
        checkIfNotDoInsert(&jsonRet, "Audio", jsonObjAudio, QJsonObject());
    }

    return jsonRet;
}

MovieInfo MovieService::getMovieInfo_mediainfo(const QFileInfo &fi)
{
    struct MovieInfo mi;

    MediaInfoDLL::MediaInfo info;
    info.Open(fi.absoluteFilePath().toStdString());
    info.Option("Complete", "1"); //??????????????????
    info.Option("Inform", "CSV"); //CSV????????????
    QString strData(info.Inform().c_str());

    //??????????????????????????????infoList?????????????????????key????????????????????????
    QStringList infoList = strData.split("\n");

    //2.????????????

    //2.1.????????????
    mi.filePath = fi.absoluteFilePath();
    mi.fileType = fi.suffix().toLower();
    mi.fileSize = fi.size();

    //2.2.???????????????
    auto videoGroup = searchGroupFromKey("Video", infoList);

    if (!videoGroup.empty()) {
        mi.valid = true;
    } else {
        mi.valid = false;
        return mi;
    }

    //????????????
    for (auto &eachPair : videoGroup) {
        if (eachPair.first == "Internet media type") {
            auto str = eachPair.second;
            auto list = str.split("/");
            if (list.size() == 2) {
                mi.vCodecID = list[1].toLower();
            } else {
                mi.vCodecID = str.toLower();
            }
            break;
        }
    }

    //?????????????????????
    int width = 0;
    int height = 0;
    for (auto &eachPair : videoGroup) {
        if (eachPair.first == "Width") {
            width = eachPair.second.toInt();
            if (width > 0) {
                break;
            }
        }
    }

    for (auto &eachPair : videoGroup) {
        if (eachPair.first == "Height") {
            height = eachPair.second.toInt();
            if (height > 0) {
                break;
            }
        }
    }

    if (width > 0 && height > 0) {
        mi.resolution = QString::number(width) + "x" + QString::number(height);
        mi.proportion = static_cast<double>(width) / height;
    }

    //??????
    for (auto &eachPair : videoGroup) {
        if (eachPair.first.indexOf("Bit rate", Qt::CaseInsensitive) != -1) {
            auto codeRate = eachPair.second.toInt();
            if (codeRate > 0) {
                mi.vCodeRate = codeRate / 1000;
                break;
            }
        }
    }

    //??????
    for (auto &eachPair : videoGroup) {
        if (eachPair.first == "Frame rate") {
            double fps = eachPair.second.toDouble();
            if (fps > 0) {
                mi.fps = static_cast<int>(fps);
                break;
            }
        }
    }

    //2.3.????????????
    for (auto &eachPair : videoGroup) {
        if (eachPair.first == "Duration") {
            auto list = eachPair.second.split(".");
            if (list.size() == 2 && std::count(list[0].begin(), list[0].end(), ':') >= 2) {
                mi.duration = list[0];
                break;
            }
        }
    }

    //2.4.????????????
    auto geGroup = searchGroupFromKey("General", infoList);
    for (auto &eachPair : geGroup) {
        if (eachPair.first == "Recorded date") {
            mi.creation = QDateTime::fromString(eachPair.second, Qt::ISODateWithMs);
            break;
        }
    }

    //2.5.????????????
    auto audioGroup = searchGroupFromKey("Audio", infoList);

    //????????????
    for (auto &eachPair : audioGroup) {
        if (eachPair.first == "Commercial name") {
            mi.aCodeID = eachPair.second.toLower();
            break;
        }
    }

    //?????????
    for (auto &eachPair : audioGroup) {
        if (eachPair.first == "Sampling rate") {
            int sampling = eachPair.second.toInt();
            if (sampling > 0) {
                mi.sampling = sampling;
                break;
            }
        }
    }

    //?????????
    for (auto &eachPair : audioGroup) {
        if (eachPair.first == "Channel(s)") {
            int channels = eachPair.second.toInt();
            if (channels > 0) {
                mi.channels = channels;
                break;
            }
        }
    }

    //??????
    for (auto &eachPair : audioGroup) {
        if (eachPair.first == "Bit rate") {
            int codeRate = eachPair.second.toInt();
            if (codeRate > 0) {
                mi.aCodeRate = codeRate / 1000;
                break;
            }
        }
    }

    //??????????????????????????????????????????
    if (mi.aCodeID != "-") {
        mi.aDigit = "8 bits";
    }

    //????????????????????????
    return mi;
}

//C-Style API
extern "C" {

    void getMovieCover(const QUrl &url, const QString &savePath, QImage *imageRet)
    {
        *imageRet = MovieService::instance()->getMovieCover(url, savePath);
    }

    void getMovieInfoByJson(const QUrl &url, QJsonObject *jsonRet)
    {
        *jsonRet = MovieService::instance()->getMovieInfoByJson(url);
    }

}
