#include "utils.h"

#include <string.h>
#include <dirent.h>

#include <fstream>

using namespace std;

map_lut utils::m_map_lut;

lutData* utils::getFilterLut(string filter)
{
    if (filter.empty())
        return nullptr;

    if (m_map_lut.find(filter) != m_map_lut.end()) {
        printf("filter:%s is finded..\n", filter.c_str());

        return &(m_map_lut[filter]);
    }

    return nullptr;
}

void utils::readFilters(const std::string& dir)
{
    if (dir.empty())
        return;

    unsigned int count=0;								//临时计数，[0，SINGLENUM]

    DIR *dp ;
    struct dirent *dirp ;

    //打开指定目录
    if((dp = opendir( dir.c_str())) == nullptr) {
        printf("open %s failed, file is not *.CUBE file, or dir does not exist.\n", dir.c_str());
        return;
    }

    m_map_lut.clear();
    lutData lut;
    //开始遍历目录
    while((dirp = readdir(dp)) != nullptr)
    {
        //跳过'.'和'..'两个目录
        if(strcmp(dirp->d_name,".")==0  || strcmp(dirp->d_name,"..")==0)
            continue;

        size_t size = strlen(dirp->d_name);

        //如果是.CUBE文件，长度至少是6
        if(size < 6)
            continue;

        //只存取.CUBE扩展名的文件名
        if(strcmp( ( dirp->d_name + (size - 5) ) , ".CUBE") != 0)
            continue;

        string filename = dir + "/" + dirp->d_name;
        if (readCubeFile(filename, lut)) {
            count++;
        }
    }

    printf("read %d CUBE files...\n", count);
}

bool utils::readCubeFile(std::string filename, lutData &lut)
{
    lut.clear();

    bool cubeDataStart = false, sizeDataStart = false;
    int lut_3d_size = 0;
    ifstream in(filename);
    std::string line;
    if (in.fail()) {
        printf("%s read fail, may be not exist!\n", filename.c_str());
        return false;
    }

    size_t n = 0;
    vector<uint8_t> rgbInt;
    while (getline(in, line)){//按行读取文件
        if(cubeDataStart) { //读取lut数据
            vector<string> rgbStr;
            split(line, " ", rgbStr);

            if (rgbStr.size() == 3) {
                lut.push_back(rgbInt);
                for (size_t i = 0; i < 3; i++)
                    lut[n].push_back(static_cast<uint8_t>(atof(rgbStr[i].c_str()) * 255));
                n++;
            }
        }

        if(sizeDataStart) { //读取lut尺寸
            sizeDataStart = false;
            string size = line.substr(sizeof("LUT_3D_SIZE"), sizeof(int));
            lut_3d_size = atoi(size.c_str());
        }

        if(strncmp(line.c_str(), "#LUT data points", sizeof("#LUT data points")) == 0) {
            cubeDataStart = true;
        }
        if(strncmp(line.c_str(), "#LUT size", sizeof("#LUT size")) == 0) {
            sizeDataStart = true;
        }
    }

    in.close();


    string filter = filename.substr(0, filename.find_last_of('.'));
    filter = filter.substr(filter.find_last_of('/') + 1);

    m_map_lut[filter] = lut;

    printf("read %s success,filtername:%s..\n", filename.c_str(), filter.c_str());

    return true;
}

void utils::split(string &str, string delimit, vector<string> &result)
{
    size_t pos = str.find(delimit);
    str += delimit;//将分隔符加入到最后一个位置，方便分割最后一位
    while (pos != string::npos) {
        result.push_back(str.substr(0, pos));
        str = str.substr(pos + 1);
        pos = str.find(delimit);
    }
}
