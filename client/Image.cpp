#define STB_IMAGE_IMPLEMENTATION
#include "Image.h"

#include <stdexcept>

Image* Image::load(const std::string &fileName) {
    Image *i = new Image();

    std::string filename = "Assets/Textures/" + fileName + ".png";

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    i->raw = stbi_load(filename.c_str(), &width, &height, &nrComponents, 4);
    i->width = width;
    i->height = height;
    i->nrComponents = nrComponents;

    if (i->raw == nullptr) {
        throw std::runtime_error("Unable to open image: " + fileName);
    }

    return i;
}