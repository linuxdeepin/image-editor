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
#ifndef EXPOSURE_H
#define EXPOSURE_H
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

#define MAX_EXPOSURE 100
#define MIN_EXPOSURE -100

/**
* @brief 曝光调节
* @param src 源数据
* @param value 调整值 -100 ～ 100
* @return 处理后的数据
*/
cv::Mat exposure(cv::Mat &src, int value = 0);

/**
* @brief 曝光调节
* @param data 数据指针
* @param width 图像宽度
* @param height 图像高度
* @param value 调整值 -100 ～ 100
*/
void exposure(unsigned char *data, const int width, const int height, int value = 0);
#endif // EXPOSURE_H
