/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     xiepengfei <xiepengfei@uniontech.com>
 *             xiepengfei <houchengqiu@uniontech.com>
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

#include "visualresult.h"

int main(int argc, char** argv)
{

    initFilters("./filter_cube");

    int i;
    for (i = 0; i < argc; i++) {
        char* arg = argv[i];
        if (i == 1) {
            // 选择的滤镜名称
            uint8_t* frame = 0;
            imageFilter24(frame, 1920, 1080, arg, 1);
        }
    }

    return 0;
}
