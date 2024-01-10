// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QtTest>
#include <gtest/gtest.h>

#include "service/ffmpegvideothumbnailer.h"

class ut_ffmpegvideothumbnailer : public testing::Test
{
public:
    void SetUp() override;
    void TearDown() override;

    bool foundLib = false;
};

void ut_ffmpegvideothumbnailer::SetUp()
{
    QString path = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    QStringList libList = QDir(path).entryList({"libffmpegthumbnailer.so*"}, QDir::NoDotAndDotDot | QDir::Files);

    foundLib = !libList.isEmpty();
}

void ut_ffmpegvideothumbnailer::TearDown() {}

TEST_F(ut_ffmpegvideothumbnailer, initFFmpegVideoThumbnailer)
{
    bool ret = initFFmpegVideoThumbnailer();
    ASSERT_EQ(ret, foundLib);
}

TEST_F(ut_ffmpegvideothumbnailer, runFFmpegVideoThumbnailer)
{
    QImage image = runFFmpegVideoThumbnailer(QUrl());
    ASSERT_TRUE(image.isNull());

    QString localFile("/usr/share/wallpapers/deepin/abc-123.jpg");
    image = runFFmpegVideoThumbnailer(QUrl::fromLocalFile(localFile));
    bool exceptExists = QFile::exists(localFile) && foundLib;
    ASSERT_EQ(exceptExists, !image.isNull());
}
