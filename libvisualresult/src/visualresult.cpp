#include "visualresult.h"
#include <string.h>

#include "utils.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

void initFilters(const char* dir)
{
    std::string path = dir;
    if (path.empty())
        path = "/usr/share/libvisualresult/filter_cube";
    utils::readFilters(path);
}

void imageFilter24(uint8_t *data, int width, int height, const char *filterName, float strength)
{
    printf("12image_filter24 called, filter is %s\n", filterName);

    uint8_t* frame = data;
    if (!frame)
        return;

    lutData* pLut = utils::getFilterLut(filterName);
    if (!pLut || pLut->size() == 0) {
        printf("filter:%s file is not found..", filterName);
        return;
    }

    lutData& lut = *pLut;

    int nRowBytes = (width * 24 + 31) / 32 * 4;
    int  lineNum_24 = 0;     //行数
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            lineNum_24 = i * nRowBytes;
            uint8_t *R = &(frame[lineNum_24 + j * 3 + 2]);
            uint8_t *G = &(frame[lineNum_24 + j * 3 + 1]);
            uint8_t *B = &(frame[lineNum_24 + j * 3 + 0]);

            unsigned int index = static_cast<unsigned int>((*R >> 3) + ((*G >> 3) << 5)  + ((*B >> 3) << 10));

            if (strength == 1.f) {
                *R = lut[index][0];
                *G = lut[index][1];
                *B = lut[index][2];
            }
            else if (strength == 0.f) {
                continue;
            }
            else {
                *R = static_cast<uint8_t>(*R * (1 - strength) + lut[index][0] * strength);
                *G = static_cast<uint8_t>(*G * (1 - strength) + lut[index][1] * strength);
                *B = static_cast<uint8_t>(*B * (1 - strength) + lut[index][2] * strength);
            }
        }
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
