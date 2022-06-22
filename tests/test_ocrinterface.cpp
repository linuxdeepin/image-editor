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
#include "service/ocrinterface.h"

TEST_F(gtestview, ocr_openImage)
{
    OcrInterface *m_ocrInterface = new OcrInterface("com.deepin.Ocr", "/com/deepin/Ocr", QDBusConnection::sessionBus(), nullptr);
    QImage img(QApplication::applicationDirPath() + "/test/jpg.jpg");
    m_ocrInterface->openImage(img);
    bool bRet = false;
    if (!img.isNull()) {
        bRet = true;
    }
    EXPECT_EQ(true, bRet);
    m_ocrInterface->deleteLater();
    m_ocrInterface = nullptr;
}

TEST_F(gtestview, ocr_openImageAndName)
{
    OcrInterface *m_ocrInterface = new OcrInterface("com.deepin.Ocr", "/com/deepin/Ocr", QDBusConnection::sessionBus(), nullptr);
    QImage img(QApplication::applicationDirPath() + "/test/jpg.jpg");
    m_ocrInterface->openImageAndName(img, QApplication::applicationDirPath() + "/test/jpg.jpg");
    bool bRet = false;
    if (!img.isNull()) {
        bRet = true;
    }
    EXPECT_EQ(true, bRet);
    m_ocrInterface->deleteLater();
    m_ocrInterface = nullptr;
}
