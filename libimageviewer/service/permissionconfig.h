// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PERMISSIONCONFIG_H
#define PERMISSIONCONFIG_H

#include <QObject>
#include <QJsonObject>

#include <dtkwidget_config.h>

#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
#include <DWaterMarkHelper>
#include <dprintpreviewsettinginfo.h>

DWIDGET_USE_NAMESPACE
#endif  // DTKWIDGET_CLASS_DWaterMarkHelper

class PermissionConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int printCount READ printCount NOTIFY printCountChanged)

    explicit PermissionConfig(QObject *parent = nullptr);
    ~PermissionConfig() override;

public:
    static PermissionConfig *instance();
    bool isValid() const;
    bool isCurrentIsTargetImage() const;

    enum Authorise {
        NoAuth = 0,
        EnableEdit = 0x1,  // 编辑权限，旋转图片是否允许写回文件
        EnableCopy = 0x2,
        EnableDelete = 0x4,
        EnableRename = 0x8,
        EnableSwitch = 0x10,  // 是否允许切换文件
        EnableWallpaper = 0x20,

        EnableReadWaterMark = 0x1000,
        EnablePrintWaterMark = 0x2000,
    };
    Q_DECLARE_FLAGS(Authorises, Authorise)

    // 权限/配置查询接口
    bool checkAuthFlag(Authorise authFlag, const QString &fileName = QString()) const;
    bool isPrintable(const QString &fileName = QString()) const;
    bool hasReadWaterMark() const;
    bool hasPrintWaterMark() const;

    enum TidType {
        TidOpen = 1000200050,
        TidEdit = 1000200051,
        TidCopy = 1000200052,
        TidPrint = 1000200053,
        TidClose = 1000200054,
        TidSwitch = 1000200055,
        TidSetWallpaper = 1000200056,
        TidDelete = 1000200057,
        TidRename = 1000200058,
    };

    // 动作触发通知接口
    void triggerAction(TidType tid, const QString &fileName);
    void triggerPrint(const QString &fileName);
    Q_SIGNAL void authoriseNotify(const QJsonObject &data);

    int printCount() const;
    bool isUnlimitPrint() const;
    Q_SIGNAL void printCountChanged();

    void setCurrentImagePath(const QString &fileName);
    Q_SIGNAL void currentImagePathChanged(const QString &fileName, bool isTargetImage);
    QString targetImage() const;

    // 阅读/打印水印
#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
    WaterMarkData readWaterMarkData() const;
    WaterMarkData printWaterMarkData() const;
#endif  // DTKWIDGET_CLASS_DWaterMarkHelper

    Q_SLOT void activateProcess(qint64 pid);
    void initFromArguments(const QStringList &arguments);

private:
    // 解析配置
    bool parseConfigOption(const QStringList &arguments, QString &configParam, QStringList &imageList) const;
    void initAuthorise(const QJsonObject &param);
    void initReadWaterMark(const QJsonObject &param);
    void initPrintWaterMark(const QJsonObject &param);
    void detectWaterMarkPluginExists();
    bool initWaterMarkPluginEnvironment();

    bool checkAuthInvalid(const QString &fileName = QString()) const;
    void reduceOnePrintCount();

    void triggerNotify(const QJsonObject &data);

private:
    enum Status { NotOpen, Open, Close };

    QString currentImagePath;  // 当前展示的图片文件路径
    QString targetImagePath;   // 权限控制指向的文件路径
    bool valid = false;
    int printLimitCount = 0;  // 打印记数，-1表示无限制
    Status status = NotOpen;  // 被控制权限图片的状态
    Authorises authFlags = NoAuth;

    bool ignoreDevicePixelRatio = false;    // 过滤设备显示比率，按照原生像素计算
    bool useWaterMarkPlugin = false;        // 是否使用水印插件
#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
    WaterMarkData readWaterMark;
    WaterMarkData printWaterMark;
#endif  // DTKWIDGET_CLASS_DWaterMarkHelper
};

#endif  // PERMISSIONCONFIG_H