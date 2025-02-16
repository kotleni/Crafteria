#ifndef BAKEDCHUNK_H
#define BAKEDCHUNK_H

#include <vector>
#include "BakedChunkPart.h"

class BakedChunk {
public:
    std::vector<BakedChunkPart> chunkParts;
    std::vector<BakedChunkPart> liquidChunkParts;
};

#endif //BAKEDCHUNK_H
