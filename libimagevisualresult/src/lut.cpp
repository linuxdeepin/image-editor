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
#include "lut.h"
#include "utils.h"

void write_lut_binary(vector<vector<float>> &lut, const int lutSize, const char *name)
{
    int rgbSize = 3 * sizeof(int);

    ofstream outFile(name, ios::out | ios::binary);

    //写入4字节LUT SIZE
    outFile.write((char*)&lutSize, sizeof(int));

    //写入LUT数据
    for (int i = 0; i < lut.size(); i++) {
        vector<int> rgb;
        rgb.push_back(lut[i][0] * 255);
        rgb.push_back(lut[i][1] * 255);
        rgb.push_back(lut[i][2] * 255);
        outFile.write((char*)&rgb[0], rgbSize);
    }
    outFile.close();
}

void parse_lut_cube(const string &filename, const char *binaryName)
{
    vector<vector<float>> lut;
    bool cubeDataStart = false, sizeDataStart = false;
    int lut3dSize = 0;
    ifstream in(filename);
    string line;

    while (getline(in, line)){//按行读取文件
        if (cubeDataStart) { //读取lut数据
            vector<string> strList;
            vector<float> rgbList;

            Libutils::split(line, " ", strList);

            for (int i = 0; i < 3; i++) {
                rgbList.push_back(atof(strList[i].c_str()));
            }

            lut.push_back(rgbList);
        }

        if (sizeDataStart) { //读取lut尺寸
            sizeDataStart = false;
            string size = line.substr(sizeof("LUT_3D_SIZE"), sizeof(int));
            lut3dSize = atoi(size.c_str());
        }

        if (strncmp(line.c_str(), "#LUT data points", sizeof("#LUT data points")) == 0) {
            cubeDataStart = true;
        }

        if (strncmp(line.c_str(), "#LUT size", sizeof("#LUT size")) == 0) {
            sizeDataStart = true;
        }
    }

    in.close();

    write_lut_binary(lut, lut3dSize, binaryName);
}
