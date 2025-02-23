#ifndef BAKEDCHUNK_H
#define BAKEDCHUNK_H

#include <SDL3/SDL_timer.h>
#include <vector>
#include "BakedChunkPart.h"

#define CHUNK_BAKE_LIFETIME_MS 3500

class BakedChunk {
public:
    std::vector<BakedChunkPart> chunkParts;
    std::vector<BakedChunkPart> liquidChunkParts;
    std::vector<BakedChunkPart> floraChunkParts;
    long bakeTime = 0L;

    BakedChunk() {
        bakeTime = SDL_GetTicks();
    }
};

#endif //BAKEDCHUNK_H
