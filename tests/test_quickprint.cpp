// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "quickprint.h"
#include "quickprint/quickprint_p.h"
#include "quickprint/printimageloader.h"
#include "gtestview.h"

#include <QImageReader>

#include <gtest/gtest.h>

class UT_QuickPrint : public ::testing::Test
{
public:
    void SetUp() override;
    void TearDown() override;
};

void UT_QuickPrint::SetUp() {}

void UT_QuickPrint::TearDown() {}

TEST_F(UT_QuickPrint, loadImageList_NormalSync_Pass)
{
    PrintImageLoader loader;
    QStringList imageList{QString(SVGPATH), QString(JPEGPATH), QApplication::applicationDirPath() + "/jpg.jpg"};
    bool ret = loader.loadImageList(imageList, false);

    EXPECT_TRUE(ret);
    EXPECT_FALSE(loader.loadData.isEmpty());

    auto takeData = loader.takeLoadData();
    EXPECT_EQ(takeData.size(), imageList.size());
    EXPECT_TRUE(loader.loadData.isEmpty());

    for (auto dataPtr : takeData) {
        EXPECT_TRUE(imageList.contains(dataPtr->filePath));
        EXPECT_EQ(dataPtr->state, ImageFileState::Loaded);
        EXPECT_EQ(dataPtr->frame, -1);
        EXPECT_FALSE(dataPtr->data.isNull());
    }

    EXPECT_FALSE(loader.isLoading());
}

TEST_F(UT_QuickPrint, loadImageList_NormalSync_Fault)
{
    PrintImageLoader loader;
    QSignalSpy spy(&loader, &PrintImageLoader::loadFinished);

    bool ret = loader.loadImageList({}, false);
    EXPECT_FALSE(ret);
    ASSERT_EQ(spy.count(), 0);

    ret = loader.loadImageList({"test", "test2"}, false);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(loader.loadData.isEmpty());

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> recvArgs = spy.takeFirst();
    EXPECT_TRUE(recvArgs.at(0).toBool());
}

TEST_F(UT_QuickPrint, loadImageList_MultiSync_Pass)
{
    PrintImageLoader loader;
    QString gif1 = QApplication::applicationDirPath() + "/gif.gif";
    QString gif2 = QApplication::applicationDirPath() + "/gif2.gif";

    QStringList imageList{gif1, gif2};
    bool ret = loader.loadImageList(imageList, false);
    EXPECT_TRUE(ret);

    QImageReader reader1(gif1);
    int count1 = reader1.imageCount();
    QImageReader reader2(gif2);
    int count2 = reader2.imageCount();

    ASSERT_EQ(loader.loadData.size(), count1 + count2);
    for (int i = 0; i < count1; ++i) {
        EXPECT_EQ(loader.loadData.at(i)->filePath, gif1);
    }
}
