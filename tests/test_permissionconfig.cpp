// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QtTest>
#include <gtest/gtest.h>

#include <DPrintPreviewDialog>

#include "service/permissionconfig.h"
#include "imageengine.h"

class UT_PermissionConfig : public ::testing::Test
{
public:
    void SetUp() override;
    void TearDown() override;
};

void UT_PermissionConfig::SetUp()
{
    PermissionConfig::instance()->valid = true;
    PermissionConfig::instance()->useWaterMarkPlugin = false;
    PermissionConfig::instance()->targetImagePath.clear();
    PermissionConfig::instance()->currentImagePath.clear();
}

void UT_PermissionConfig::TearDown()
{
    PermissionConfig::instance()->valid = false;
    PermissionConfig::instance()->useWaterMarkPlugin = false;
}

static QString permission_config()
{
    QByteArray permission("{ \"permission\": { \"edit\": true, \"copy\": true, \"setWallpaper\": true, \"pictureSwitch\": true, \"printCount\": 2 } }");
    return permission.toBase64();
}

TEST_F(UT_PermissionConfig, parseConfigOption_NormalParam_Pass)
{
    QString configParam;
    QStringList imageList;
    QString tempPath = QApplication::applicationDirPath() + "/svg.svg";
    QStringList tmpList {tempPath};
    QStringList arguments {qApp->arguments().first(), "--config=test", tempPath};

    EXPECT_TRUE(PermissionConfig::instance()->parseConfigOption(arguments, configParam, imageList));
    EXPECT_EQ(configParam, QString("test"));
    EXPECT_EQ(imageList, tmpList);

    QStringList arguments2 {qApp->arguments().first(), "--config", "test", tempPath};
    EXPECT_TRUE(PermissionConfig::instance()->parseConfigOption(arguments2, configParam, imageList));
    EXPECT_EQ(configParam, QString("test"));
}

TEST_F(UT_PermissionConfig, parseConfigOption_NormalParam_Fail)
{
    QString configParam;
    QStringList imageList;
    QString tempPath = QApplication::applicationDirPath() + "/svg.svg";
    QStringList arguments {qApp->arguments().first(), "config", "test", tempPath};

    EXPECT_FALSE(PermissionConfig::instance()->parseConfigOption({}, configParam, imageList));
    EXPECT_TRUE(configParam.isEmpty());
    EXPECT_TRUE(imageList.isEmpty());

    QStringList tmpList {"config", "test", tempPath};
    EXPECT_FALSE(PermissionConfig::instance()->parseConfigOption(arguments, configParam, imageList));
    EXPECT_TRUE(configParam.isEmpty());
    EXPECT_EQ(imageList, tmpList);
}

TEST_F(UT_PermissionConfig, initFromArguments_NormalParam_Pass)
{
    PermissionConfig::instance()->valid = false;
    PermissionConfig::instance()->authFlags = PermissionConfig::NoAuth;

    QString tempPath = QApplication::applicationDirPath() + "/svg.svg";
    QStringList args;
    args << qApp->arguments().first()
         << QString("--config=%1").arg(permission_config())
         << tempPath;

    PermissionConfig::instance()->initFromArguments(args);
    EXPECT_TRUE(PermissionConfig::instance()->isValid());

    PermissionConfig::Authorises auth = PermissionConfig::Authorises(
        PermissionConfig::EnableCopy | PermissionConfig::EnableEdit | PermissionConfig::EnableSwitch | PermissionConfig::EnableWallpaper);
    EXPECT_EQ(PermissionConfig::instance()->authFlags, auth);
    EXPECT_EQ(PermissionConfig::instance()->targetImagePath, tempPath);
}

TEST_F(UT_PermissionConfig, initFromArguments_NormalParam_Fail)
{
    PermissionConfig::instance()->valid = false;
    PermissionConfig::instance()->authFlags = PermissionConfig::NoAuth;

    PermissionConfig::instance()->initFromArguments({});
    EXPECT_FALSE(PermissionConfig::instance()->isValid());
    EXPECT_EQ(PermissionConfig::instance()->authFlags, PermissionConfig::NoAuth);
    EXPECT_TRUE(PermissionConfig::instance()->targetImagePath.isEmpty());
}

TEST_F(UT_PermissionConfig, checkAuthFlag_NotInit_Pass)
{
    PermissionConfig::instance()->authFlags = PermissionConfig::Authorises(0b11000000111111);
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableEdit));
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableCopy));
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableDelete));
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableRename));
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableSwitch));
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableWallpaper));
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableReadWaterMark));
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnablePrintWaterMark));
    EXPECT_TRUE(PermissionConfig::instance()->hasReadWaterMark());
    EXPECT_TRUE(PermissionConfig::instance()->hasPrintWaterMark());

    PermissionConfig::instance()->currentImagePath = "1.png";
    PermissionConfig::instance()->targetImagePath = "1.png";
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthInvalid());
    EXPECT_TRUE(PermissionConfig::instance()->checkAuthInvalid("2.png"));
}

TEST_F(UT_PermissionConfig, checkAuthFlag_NotInit_Fail)
{
    EXPECT_TRUE(PermissionConfig::instance()->valid);
    PermissionConfig::instance()->authFlags = PermissionConfig::NoAuth;
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableEdit));
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableCopy));
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableDelete));
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableRename));
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableSwitch));
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableWallpaper));
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnableReadWaterMark));
    EXPECT_FALSE(PermissionConfig::instance()->checkAuthFlag(PermissionConfig::EnablePrintWaterMark));
    EXPECT_FALSE(PermissionConfig::instance()->hasReadWaterMark());
    EXPECT_FALSE(PermissionConfig::instance()->hasPrintWaterMark());
}

TEST_F(UT_PermissionConfig, print_Count_Pass)
{
    QString tmpPath("/tmp/tmp.png");
    PermissionConfig::instance()->targetImagePath = tmpPath;

    PermissionConfig::instance()->printLimitCount = 1;
    EXPECT_EQ(PermissionConfig::instance()->printLimitCount, 1);
    EXPECT_TRUE(PermissionConfig::instance()->isPrintable(tmpPath));

    PermissionConfig::instance()->triggerPrint(tmpPath);
    EXPECT_EQ(PermissionConfig::instance()->printLimitCount, 0);
    EXPECT_FALSE(PermissionConfig::instance()->isPrintable(tmpPath));

    PermissionConfig::instance()->printLimitCount = -1;
    EXPECT_TRUE(PermissionConfig::instance()->isPrintable(tmpPath));
    PermissionConfig::instance()->triggerPrint(tmpPath);
    EXPECT_EQ(PermissionConfig::instance()->printLimitCount, -1);
    EXPECT_TRUE(PermissionConfig::instance()->isUnlimitPrint());
}

TEST_F(UT_PermissionConfig, watarMark_JsonData_Pass)
{
    // Default value
    auto markData = PermissionConfig::instance()->readWaterMarkData();
    EXPECT_EQ(markData.type, WaterMarkType::None);
    EXPECT_EQ(markData.opacity, 1);
    EXPECT_EQ(markData.layout, WaterMarkLayout::Center);
    EXPECT_TRUE(markData.text.isEmpty());

    QJsonObject param {{"text", "test"}, {"opacity", 0}, {"layout", 1}};

    PermissionConfig::instance()->initReadWaterMark(param);
    markData = PermissionConfig::instance()->readWaterMarkData();
    EXPECT_EQ(markData.type, WaterMarkType::Text);
    EXPECT_EQ(markData.opacity, 0);
    EXPECT_EQ(markData.layout, WaterMarkLayout::Tiled);
    EXPECT_EQ(markData.text, QString("test"));

    PermissionConfig::instance()->initPrintWaterMark(param);
    markData = PermissionConfig::instance()->printWaterMarkData();
    EXPECT_EQ(markData.type, WaterMarkType::Text);
}

TEST_F(UT_PermissionConfig, triggerAction_SignalNotify_Pass)
{
    QJsonObject notifyData;
    QObject::connect(PermissionConfig::instance(), &PermissionConfig::authoriseNotify, [&](const QJsonObject &notify){
        notifyData = notify;
    });

    auto TriggerFunction = [&](PermissionConfig::TidType tid, const QString &operate){
        // PermissionConfig::targetImagePath is empty.
        PermissionConfig::instance()->triggerAction(tid, "");
        // ReportMode::ReportAndBroadcast = 0b11
        auto policyData = notifyData.value("policy").toObject();
        EXPECT_EQ(policyData.value("reportMode").toInt(), 3);

        QJsonObject data = notifyData.value("info").toObject();
        EXPECT_EQ(data.value("tid").toInt(), tid);
        EXPECT_EQ(data.value("operate").toString(), operate);
    };

    PermissionConfig::instance()->status = PermissionConfig::NotOpen;

    TriggerFunction(PermissionConfig::TidOpen, "open");
    TriggerFunction(PermissionConfig::TidEdit, "edit");
    TriggerFunction(PermissionConfig::TidCopy, "copy");
    // Close will set PermissionConfig::valid to false.
    TriggerFunction(PermissionConfig::TidClose, "close");
    EXPECT_FALSE(PermissionConfig::instance()->isValid());
    EXPECT_EQ(PermissionConfig::instance()->status, PermissionConfig::Close);
}

TEST_F(UT_PermissionConfig, hasPrintWaterMark_usingWaterMark_Pass)
{
    PermissionConfig::instance()->authFlags = PermissionConfig::EnablePrintWaterMark;
    EXPECT_TRUE(PermissionConfig::instance()->hasPrintWaterMark());

    PermissionConfig::instance()->useWaterMarkPlugin = true;
    EXPECT_FALSE(PermissionConfig::instance()->hasPrintWaterMark());
}

TEST_F(UT_PermissionConfig, detectWaterMarkPluginExists_Pass)
{
    PermissionConfig::instance()->detectWaterMarkPluginExists();

    QStringList plugins = DPrintPreviewDialog::availablePlugins();
    if (plugins.contains("WaterMarkFilter")) {
        EXPECT_TRUE(PermissionConfig::instance()->useWaterMarkPlugin);
    } else {
        EXPECT_FALSE(PermissionConfig::instance()->useWaterMarkPlugin);
    }
}

TEST_F(UT_PermissionConfig, initWaterMarkPluginEnvironment_CheckPlugin_Pass)
{
    DWIDGET_USE_NAMESPACE
    QStringList plugins = DPrintPreviewDialog::availablePlugins();
    if (plugins.contains("WaterMarkFilter")) {
        EXPECT_TRUE(PermissionConfig::instance()->initWaterMarkPluginEnvironment());
    } else {
        EXPECT_FALSE(PermissionConfig::instance()->initWaterMarkPluginEnvironment());
    }

    QByteArray envData = qgetenv("DEEPIN_WATERMARK");
    EXPECT_FALSE(envData.isEmpty());
}
