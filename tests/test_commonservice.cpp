/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     LiuMingHang <liuminghang@uniontech.com>
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
#include "gtestview.h"
#include "service/commonservice.h"

TEST_F(gtestview, cp2Image)
{
    for (int i = 0; i < 200; i++) {
        QFile::copy(":/jpg.jpg", QApplication::applicationDirPath() + "/test/jpg" + QString::number(i) + ".jpg");
        QFile(QApplication::applicationDirPath() + "/test/jpg" + QString::number(i) + ".jpg").setPermissions(\
                                                                                                             QFile::WriteUser | QFile::ReadUser | QFile::WriteOther | \
                                                                                                             QFile::ReadOther | QFile::ReadGroup | QFile::WriteGroup);
        EXPECT_EQ(true, QFileInfo(QApplication::applicationDirPath() + "/test/jpg" + QString::number(i) + ".jpg").isFile());
    }

}

TEST_F(gtestview, commonservice_reName)
{
    QString oldName = QApplication::applicationDirPath() + "/test/jpg.jpg";
    QString newName = QApplication::applicationDirPath() + "/test/jpgxxx.jpg";

    LibCommonService::instance()->reName(oldName, newName);
//    EXPECT_EQ(true, bRet);
}

