#define STB_IMAGE_IMPLEMENTATION
#include "Image.h"

#include <iostream>
#include <ostream>
#include <stdexcept>

#define NR_COMPONENTS_COUNT 4

Image* Image::load(const std::string &fileName) {
    Image *i = new Image();

    std::string filename = "Assets/Textures/" + fileName + ".png";

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    i->raw = stbi_load(filename.c_str(), &width, &height, &nrComponents, 4);
    i->width = width;
    i->height = height;
    i->nrComponents = nrComponents;

    if (nrComponents != NR_COMPONENTS_COUNT) {
        std::cerr << "Warning! Wrong number of texture nrComponents: " << nrComponents << " for " << filename << std::endl;
    }

    if (i->raw == nullptr) {
        throw std::runtime_error("Unable to open image: " + fileName);
    }

    return i;
}