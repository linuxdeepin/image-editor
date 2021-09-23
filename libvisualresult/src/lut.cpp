#include "lut.h"


//按字符“delimit”分割字符串
void split(string &str, string delimit, vector<string>&result)
{
    size_t pos = str.find(delimit);
    str += delimit;//将分隔符加入到最后一个位置，方便分割最后一位
    while (pos != string::npos) {
        result.push_back(str.substr(0, pos));
        str = str.substr(pos + 1);
        pos = str.find(delimit);
    }
}

int read_lut_binary(const char *name, vector<vector<float>> *lut)
{
    int rgb_size = 3 * sizeof(float);
    ifstream inFile(name, ios::in | ios::binary);

    if(!inFile){
        cout<< "error" <<endl;
        return -1;
    }

    //先读尺寸
    int lut_size = -1;
    inFile.read((char *)&lut_size, sizeof(int));

    //再读数据
    float temp[3];
    while(inFile.read((char *)&temp[0], rgb_size)){
        vector<float> item;
        for (int i = 0; i < 3; i++) {
            item.push_back(temp[i]);
        }
        lut->push_back(item);
    }
    inFile.close();

    return lut_size;
}

void write_lut_binary(vector<vector<float>> &lut, int lut_size, const char *name)
{
    int rgb_size = 3 * sizeof(float);

    ofstream outFile(name, ios::out | ios::binary);

    //写入4字节LUT SIZE
    outFile.write((char*)&lut_size, sizeof(int));

    //写入LUT数据
    for (int i = 0; i < lut.size(); i++)
        outFile.write((char*)&lut[i][0], rgb_size);

    outFile.close();
}

//读取lut文件
void read_lut_file(string filename)
{
    vector<vector<float>> lut;
    bool cubeDataStart = false, sizeDataStart = false;
    int lut_3d_size = 0;
    ifstream in(filename);
    string line;

    while (getline(in, line)){//按行读取文件
        if(cubeDataStart) { //读取lut数据
            vector<string> str_list;
            vector<float> rgb_list;

            split(line, " ", str_list);

            /*rgb_list.push_back(atof(str_list[0].c_str()));
            rgb_list.push_back(atof(str_list[1].c_str()));
            rgb_list.push_back(atof(str_list[2].c_str()));*/

            for (int i = 0; i < 3; i++) {
                rgb_list.push_back(atof(str_list[i].c_str()));
            }

            lut.push_back(rgb_list);
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

    write_lut_binary(lut, lut_3d_size, "test.dat");
}


