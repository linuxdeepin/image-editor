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
#endif

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
        EnableEdit = 0x1,
        EnableCopy = 0x2,
        EnableDelete = 0x4,
        EnableRename = 0x8,
        EnableSwitch = 0x10,

        EnableReadWaterMark = 0x1000,
        EnablePrintWaterMark = 0x2000,
    };
    Q_DECLARE_FLAGS(Authorises, Authorise)

    bool isEditable(const QString &fileName = QString()) const;
    bool isCopyable(const QString &fileName = QString()) const;
    bool isDeletable() const;
    bool isRenamable() const;
    bool isSwitchable() const;
    bool isPrintable(const QString &fileName = QString()) const;
    bool hasReadWaterMark() const;
    bool hasPrintWaterMark() const;

    void triggerOpen(const QString &fileName);
    void triggerEdit(const QString &fileName);
    void triggerCopy(const QString &fileName);
    void triggerDelete(const QString &fileName);
    void triggerRename(const QString &fileName);
    void triggerPrint(const QString &fileName);
    void triggerClose(const QString &fileName);
    Q_SIGNAL void authoriseNotify(const QJsonObject &data);

    int printCount() const;
    bool isUnlimitPrint() const;
    Q_SIGNAL void printCountChanged();

    void setCurrentImagePath(const QString &fileName);
    QString targetImage() const;

#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
    WaterMarkData readWaterMarkData() const;
    WaterMarkData printWaterMarkData() const;
#endif

    void initFromArguments();

private:
    QString parseConfigOption();
    void initAuthorise(const QJsonObject &param);
    void initReadWaterMark(const QJsonObject &param);
    void initPrintWaterMark(const QJsonObject &param);
    bool checkAuthInvalid(const QString &fileName = QString()) const;
    void reduceOnePrintCount();
    void triggerNotify(const QJsonObject &data);

private:
    enum TidType {
        TidOpen = 1000200050,
        TidEdit = 1000200051,
        TidCopy = 1000200052,
        TidPrint = 1000200053,
        TidClose = 1000200054,
        TidDelete = 1000200055,
        TidRename = 1000200056,
    };
    enum Status { NotOpen, Open, Close };

    QString currentImagePath;  // 当前展示的图片文件路径
    QString targetImagePath;   // 权限控制指向的文件路径
    bool valid = false;
    int printLimitCount = 0;
    Status status = NotOpen;  // 被控制权限图片的状态
    Authorises authFlags = NoAuth;
#ifdef DTKWIDGET_CLASS_DWaterMarkHelper
    WaterMarkData readWaterMark;
    WaterMarkData printWaterMark;
#endif
};

#endif  // PERMISSIONCONFIG_H
