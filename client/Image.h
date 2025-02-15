#ifndef H_IMAGE
#define H_IMAGE

#include <string>

#include "stb_image.h"

class Image {
public:
    int width, height, nrComponents;
    unsigned char *raw;

    static Image *load(const std::string &file);
};

#endif