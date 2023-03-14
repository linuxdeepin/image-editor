// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "permissionconfig.h"
#include "imageengine.h"

#include <QApplication>
#include <QJsonDocument>
#include <QFileInfo>
#include <QDebug>

const QString g_KeyTid = "tid";
const QString g_KeyOperate = "operate";
const QString g_KeyFilePath = "filePath";
const QString g_KeyRemaining = "remainingPrintCount";

/**
   @class AuthoriseConfig
   @brief 授权控制类，提供操作授权和水印配置
   @details 授权控制主要包括编辑、拷贝、删除、保存等操作，水印包括阅读水印及打印水印。
    配置信息通过命令行参数获取，当授权控制开启时，进行主要操作将自动发送通知信息。
   @note 此类非线程安全
 */

/**
   @brief 构造函数，构造时即初始化配置
 */
PermissionConfig::PermissionConfig(QObject *parent)
    : QObject(parent)
{
#ifdef DISABLE_WATERMARK
    qWarning() << qPrintable("Current version is not support read watermark");
#endif
}

/**
   @brief 析构函数
 */
PermissionConfig::~PermissionConfig() {}

/**
   @return 返回权限控制单实例
 */
PermissionConfig *PermissionConfig::instance()
{
    static PermissionConfig config;
    return &config;
}

/**
   @return 是否允许进行权限控制，未加载授权配置时返回 false
 */
bool PermissionConfig::isValid() const
{
    return valid;
}

/**
   @return 返回当前是否为权限控制目标图片
 */
bool PermissionConfig::isCurrentIsTargetImage() const
{
    return isValid() && currentImagePath == targetImagePath;
}

/**
   @return 是否允许编辑图片，无授权控制时默认返回 true
 */
bool PermissionConfig::isEditable(const QString &fileName) const
{
    if (checkAuthInvalid(fileName)) {
        return true;
    }

    return authFlags.testFlag(EnableEdit);
}

/**
   @return 是否允许复制图片，无授权控制时默认返回 true
 */
bool PermissionConfig::isCopyable(const QString &fileName) const
{
    if (checkAuthInvalid(fileName)) {
        return true;
    }

    return authFlags.testFlag(EnableCopy);
}

/**
   @return 是否允许删除图片，无授权控制时默认返回 true
 */
bool PermissionConfig::isDeletable() const
{
    if (checkAuthInvalid()) {
        return true;
    }

    return authFlags.testFlag(EnableDelete);
}

/**
   @return 是否允许重命名图片，无授权控制时默认返回 true
 */
bool PermissionConfig::isRenamable() const
{
    if (checkAuthInvalid()) {
        return true;
    }

    return authFlags.testFlag(EnableRename);
}

/**
   @return 是否允许切换图片，无授权控制时默认返回 true
 */
bool PermissionConfig::isSwitchable() const
{
    if (checkAuthInvalid()) {
        return true;
    }

    return authFlags.testFlag(EnableSwitch);
}

/**
   @return 是否允许打印图片，无授权控制时默认返回 true
   @note -1 表示无限制;0 表示无打印次数;1~表示剩余可打印次数
 */
bool PermissionConfig::isPrintable(const QString &fileName) const
{
    if (checkAuthInvalid(fileName)) {
        return true;
    }

    return !!printLimitCount;
}

/**
   @return 是否存在阅读水印
 */
bool PermissionConfig::hasReadWaterMark() const
{
    return authFlags.testFlag(EnableReadWaterMark);
}

/**
   @return 是否存在打印水印
 */
bool PermissionConfig::hasPrintWaterMark() const
{
    return authFlags.testFlag(EnablePrintWaterMark);
}

/**
   @brief 触发打开文件 \a fileName ，若为限权文件，向外发送权限通知信号
 */
void PermissionConfig::triggerOpen(const QString &fileName)
{
    if (checkAuthInvalid(fileName)) {
        return;
    }

    if (NotOpen != status) {
        return;
    }
    status = Open;

    QJsonObject data{{g_KeyTid, TidOpen}, {g_KeyOperate, "open"}, {g_KeyFilePath, fileName}};

    triggerNotify(data);
}

/**
   @brief 触发编辑文件 \a fileName ，若为限权文件，向外发送权限通知信号
 */
void PermissionConfig::triggerEdit(const QString &fileName)
{
    if (checkAuthInvalid(fileName)) {
        return;
    }

    QJsonObject data{{g_KeyTid, TidEdit}, {g_KeyOperate, "edit"}, {g_KeyFilePath, fileName}};

    triggerNotify(data);
}

/**
   @brief 触发拷贝文件 \a fileName ，若为限权文件，向外发送权限通知信号
 */
void PermissionConfig::triggerCopy(const QString &fileName)
{
    if (checkAuthInvalid(fileName)) {
        return;
    }

    QJsonObject data{{g_KeyTid, TidCopy}, {g_KeyOperate, "copy"}, {g_KeyFilePath, fileName}};

    triggerNotify(data);
}

/**
   @brief 触发删除文件 \a fileName ，若为限权文件，向外发送权限通知信号
 */
void PermissionConfig::triggerDelete(const QString &fileName)
{
    if (checkAuthInvalid(fileName)) {
        return;
    }

    QJsonObject data{{g_KeyTid, TidDelete}, {g_KeyOperate, "delete"}, {g_KeyFilePath, fileName}};

    triggerNotify(data);
}

/**
   @brief 触发重命名文件 \a fileName ，若为限权文件，向外发送权限通知信号
 */
void PermissionConfig::triggerRename(const QString &fileName)
{
    if (checkAuthInvalid()) {
        return;
    }

    QJsonObject data{{g_KeyTid, TidRename}, {g_KeyOperate, "rename"}, {g_KeyFilePath, fileName}};

    triggerNotify(data);
}

/**
   @brief 触发打印文件 \a fileName ，若为限权文件，向外发送权限通知信号
 */
void PermissionConfig::triggerPrint(const QString &fileName)
{
    if (checkAuthInvalid()) {
        return;
    }

    // 减少打印计数
    reduceOnePrintCount();
    QJsonObject data{{g_KeyTid, TidPrint}, {g_KeyOperate, "print"}, {g_KeyFilePath, fileName}, {g_KeyRemaining, printCount()}};

    triggerNotify(data);
}

/**
   @brief 触发关闭文件 \a fileName ，若为限权文件，向外发送权限通知信号
 */
void PermissionConfig::triggerClose(const QString &fileName)
{
    if (checkAuthInvalid(fileName)) {
        return;
    }

    if (Open != status) {
        return;
    }
    status = Close;

    QJsonObject data{{"tid", TidClose}, {g_KeyOperate, "close"}, {g_KeyFilePath, fileName}};

    triggerNotify(data);
}

/**
   @brief 返回当前剩余的打印次数
   @sa `isPrintable`
 */
int PermissionConfig::printCount() const
{
    return printLimitCount;
}

/**
   @brief 返回是否打印无限制
 */
bool PermissionConfig::isUnlimitPrint() const
{
    if (checkAuthInvalid()) {
        return true;
    }
    return -1 == printLimitCount;
}

/**
   @brief 减少一次打印计数并发送打印计数变更信号 `printCountChanged`
 */
void PermissionConfig::reduceOnePrintCount()
{
    if (printLimitCount > 0) {
        printLimitCount--;
        Q_EMIT printCountChanged();
    } else {
        qWarning() << "Escape print authorise check!";
    }
}

/**
   @brief 触发权限操作通知，将向外发送Json数据，通过DBus广播
 */
void PermissionConfig::triggerNotify(const QJsonObject &data)
{
    enum ReportMode { Broadcast = 1, Report = 2, ReportAndBroadcast = Broadcast | Report };
    QJsonObject sendData;
    sendData.insert("policy", QJsonObject{{"reportMode", ReportAndBroadcast}});
    sendData.insert("info", data);

    Q_EMIT authoriseNotify(sendData);
}

/**
   @brief 设置当前处理的文件路径为 \a fileName
 */
void PermissionConfig::setCurrentImagePath(const QString &fileName)
{
    currentImagePath = fileName;
}

/**
   @return 返回当前控制的图片路径
 */
QString PermissionConfig::targetImage() const
{
    return targetImagePath;
}

#ifndef DISABLE_WATERMARK

/**
   @return 返回从配置中读取的阅读水印配置，用于图片展示时显示
 */
WaterMarkData PermissionConfig::readWaterMarkData() const
{
    return readWaterMark;
}

/**
   @return 返回从配置中读取的打印水印配置，用于图片打印预览及打印时显示
 */
WaterMarkData PermissionConfig::printWaterMarkData() const
{
    return printWaterMark;
}

#endif

/**
   @brief 从命令行参数中取得授权配置
   @note 命令行参数示例：
    deepin-image-viewer --config=[Base64 code data] /path/to/file
 */
void PermissionConfig::initFromArguments()
{
    if (valid) {
        return;
    }

    QString configParam = parseConfigOption();
    if (!configParam.isEmpty()) {
        // 获取带权限控制的文件路径
        QStringList arguments = qApp->arguments();
        for (const QString &arg : arguments) {
            QFileInfo info(arg);
            QString filePath = info.absoluteFilePath();
            if (ImageEngine::instance()->isImage(filePath) || info.suffix() == "dsps") {
                targetImagePath = filePath;
                break;
            }
        }

        if (targetImagePath.isEmpty()) {
            qWarning() << qPrintable("Authorise config with no target image path.") << arguments;
            return;
        }

        QByteArray jsonData = QByteArray::fromBase64(configParam.toUtf8());
        qInfo() << QString("Parse authorise config, data: %1").arg(QString(jsonData));

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);

        if (!doc.isNull()) {
            QJsonObject root = doc.object();
            initAuthorise(root.value("permission").toObject());
            initReadWaterMark(root.value("readWatermark").toObject());
            initPrintWaterMark(root.value("printWatermark").toObject());

            valid = !targetImagePath.isEmpty();
            if (valid) {
                // 首次触发打开图片
                triggerOpen(targetImagePath);
            }

            qInfo() << qPrintable("Current Enable permission") << authFlags;
        } else {
            qWarning()
                << QString("Parse authorise config error at pos: %1, details: %2").arg(error.offset).arg(error.errorString());
        }
    } else {
        qWarning() << qPrintable("Parse authorise config is empty.");
    }
}

/**
   @return 解析命令行参数并返回设置的权限和水印配置，为设置则返回空
 */
QString PermissionConfig::parseConfigOption()
{
    if (!qApp) {
        qWarning() << qPrintable("Must init authorise config after QApplication initialized");
        return {};
    }

    const QString option("--config");
    const QLatin1Char assignChar('=');
    enum Status { FindOption, FindAssign, FindParam };
    Status status = FindOption;

    QStringList arguments = qApp->arguments();
    for (const QString &arg : arguments) {
        switch (status) {
            case FindOption:
                if (arg.simplified().startsWith(option)) {
                    int index = arg.indexOf(assignChar);
                    if (index < 0) {
                        status = FindAssign;
                        continue;
                    } else if (index == arg.length() - 1) {
                        status = FindParam;
                        continue;
                    } else {
                        return arg.mid(index + 1);
                    }
                }
                break;
            case FindAssign:
                if (arg.simplified().startsWith(assignChar)) {
                    if (arg.length() > 1) {
                        return arg.mid(1);
                    } else {
                        status = FindParam;
                        continue;
                    }
                } else {
                    return {};
                }
                break;
            case FindParam:
                return arg;
        }
    }

    return {};
}

/**
   @brief 从 Json 配置 \a param 中取得授权信息
 */
void PermissionConfig::initAuthorise(const QJsonObject &param)
{
    if (param.isEmpty()) {
        qInfo() << qPrintable("Authorise config not contains authorise data.");
        return;
    }

    // 屏蔽 delete / rename ，默认无此功能
    authFlags.setFlag(EnableEdit, param.value("edit").toBool(false));
    authFlags.setFlag(EnableCopy, param.value("copy").toBool(false));
    // 默认允许切换图片，包括快捷键及 Ctrl + O
    authFlags.setFlag(EnableSwitch, true);

    printLimitCount = param.value("printCount").toInt(0);
}

/**
   @brief 从 Json 配置 \a param 中取得阅读水印信息
 */
void PermissionConfig::initReadWaterMark(const QJsonObject &param)
{
    if (param.isEmpty()) {
        qInfo() << qPrintable("Authorise config not contains read watermark data.");
        return;
    }

#ifndef DISABLE_WATERMARK
    readWaterMark.type = WaterMarkType::Text;
    readWaterMark.font.setFamily(param.value("font").toString());
    readWaterMark.font.setPixelSize(param.value("fontSize").toInt());

    QString colorName = param.value("color").toString();
    if (!colorName.startsWith('#')) {
        colorName.prepend('#');
    }
    readWaterMark.color.setNamedColor(colorName);
    readWaterMark.opacity = param.value("opacity").toDouble() / 255;
    readWaterMark.layout = param.value("layout").toInt() ? WaterMarkLayout::Tiled : WaterMarkLayout::Center;
    readWaterMark.rotation = param.value("angle").toDouble();
    readWaterMark.lineSpacing = param.value("rowSpacing").toInt();
    readWaterMark.spacing = param.value("columnSpacing").toInt();
    readWaterMark.text = param.value("text").toString();

    authFlags.setFlag(EnableReadWaterMark, true);
#endif
}

/**
   @brief 从 Json 配置 \a param 中取得打印水印信息
 */
void PermissionConfig::initPrintWaterMark(const QJsonObject &param)
{
    if (param.isEmpty()) {
        qInfo() << qPrintable("Authorise config not contains print watermark data.");
        return;
    }

#ifndef DISABLE_WATERMARK
    printWaterMark.type = WaterMarkType::Text;
    printWaterMark.font.setFamily(param.value("font").toString());
    printWaterMark.font.setPixelSize(param.value("fontSize").toInt());

    QString colorName = param.value("color").toString();
    if (!colorName.startsWith('#')) {
        colorName.prepend('#');
    }
    printWaterMark.color.setNamedColor(colorName);
    printWaterMark.opacity = param.value("opacity").toDouble() / 255;
    printWaterMark.layout = param.value("layout").toInt() ? WaterMarkLayout::Tiled : WaterMarkLayout::Center;
    printWaterMark.rotation = param.value("angle").toDouble();
    printWaterMark.lineSpacing = param.value("rowSpacing").toInt();
    printWaterMark.spacing = param.value("columnSpacing").toInt();
    printWaterMark.text = param.value("text").toString();

    authFlags.setFlag(EnablePrintWaterMark, true);
#endif
}

/**
   @return 返回文件 \a fileName 是否需要被校验权限
 */
bool PermissionConfig::checkAuthInvalid(const QString &fileName) const
{
    if (!isValid()) {
        return true;
    }
    if (fileName.isEmpty()) {
        return currentImagePath != targetImagePath;
    }

    return fileName != targetImagePath;
}
