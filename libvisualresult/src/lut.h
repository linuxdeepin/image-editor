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
#ifndef LUT_H
#define LUT_H

#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

/**
* @brief 从二进制文件读取3dLut
* @param name 文件名
* @param lut 指向相应LUT的指针
* @return LUT size
*/
int read_lut_binary(const char *name, vector<vector<int>> *lut);

/**
* @brief 将3dLut写入二进制文件
* @param lut 指向相应LUT的指针
* @param lut_size LUT size
* @param name 文件名
*/
void write_lut_binary(vector<vector<float>> &lut, const int lut_size, const char *name);

/**
* @brief 解析.CUBE文件
* @param filename CUBE文件名
* @param binary_name 二进制文件名
*/
void parse_lut_cube(const string &filename, const char *binary_name);

#endif // LUT_H
