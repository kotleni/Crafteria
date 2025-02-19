#ifndef ABSTRACTWORLDGENERATOR_H
#define ABSTRACTWORLDGENERATOR_H

#include "../../PerlinNoise.h"
#include "../Chunk.h"

class AbstractWorldGenerator {
public:
    virtual ~AbstractWorldGenerator() = default;

    int seed;
    siv::PerlinNoise perlin;

    explicit AbstractWorldGenerator(int seed) {
        this->seed = seed;
        this->perlin = siv::PerlinNoise(seed);
    }

    virtual void generateChunk(Chunk *chunk) = 0;
};

#endif //ABSTRACTWORLDGENERATOR_H
