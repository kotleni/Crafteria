#ifndef BAKEDCHUNK_H
#define BAKEDCHUNK_H

#include <SDL_timer.h>
#include <vector>
#include "BakedChunkPart.h"

#define CHUNK_BAKE_LIFETIME_MS 3500

class BakedChunk {
public:
    std::vector<BakedChunkPart> chunkParts;
    std::vector<BakedChunkPart> liquidChunkParts;
    long bakeTime = 0L;

    BakedChunk() {
        bakeTime = SDL_GetTicks();
    }

    bool isOutdated() {
        return (SDL_GetTicks() - bakeTime) > CHUNK_BAKE_LIFETIME_MS;
    }
};

#endif //BAKEDCHUNK_H
