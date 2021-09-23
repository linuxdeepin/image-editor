#ifndef LUT_H
#define LUT_H

#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

void split(string &str, string delimit, vector<string>&result);

//读出二进制文件,返回lut size
int read_lut_binary(const char *name, vector<vector<float>> *lut);

//写入二进制文件
void write_lut_binary(vector<vector<float>> &lut, int lut_size, const char *name);

//读取并解析LUT文件
void read_lut_file(string filename);


#endif // LUT_H
