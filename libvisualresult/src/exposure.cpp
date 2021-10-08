/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     fengli <fengli@uniontech.com>
 *
 * Maintainer: liuzheng <liuzheng@uniontech.com>
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

#include "exposure.h"
cv::Mat exposure(cv::Mat &src, int value)
{
    cv::Mat ret;
    src.copyTo(ret);

    if(value == 0)
        return ret;

    if(value > MAX_EXPOSURE || value < MIN_EXPOSURE)
        value = 0;

    float step = value / 100.0;
    for (int i = 0; i < ret.rows; i++) {
        for (int j = 0; j < ret.cols; j++) {
            ret.at<cv::Vec3b>(i, j)[1] = cv::saturate_cast<uchar>(ret.at<cv::Vec3b>(i, j)[1] * pow(2, step)); // green
            ret.at<cv::Vec3b>(i, j)[2] = cv::saturate_cast<uchar>(ret.at<cv::Vec3b>(i, j)[2] * pow(2, step)); // red
            ret.at<cv::Vec3b>(i, j)[0] = cv::saturate_cast<uchar>(ret.at<cv::Vec3b>(i, j)[0] * pow(2, step)); // blue
        }
    }
    return ret;
}

void exposure(unsigned char *data, const int width, const int height, int value)
{
    if(value > MAX_EXPOSURE || value < MIN_EXPOSURE)
        value = 0;
    if(value == 0)
        return;

    float step = value / 100.0;
    int size = width * height;

    for(int i = 0; i < size; i++) {
        int r = data[i*3] * pow(2, step);
        int g = data[i*3 + 1] * pow(2, step);
        int b = data[i*3 + 2] * pow(2, step);

        data[i*3] = r > 255 ? 255 : r;
        data[i*3 + 1] = g > 255 ? 255 : g;
        data[i*3 + 2] = b > 255 ? 255 : b;
    }
}
