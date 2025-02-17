#ifndef DEFAULTWORLDGENERATOR_H
#define DEFAULTWORLDGENERATOR_H

#include "../../constants.h"
#include "AbstractWorldGenerator.h"

class DefaultWorldGenerator: public AbstractWorldGenerator {
public:
    DefaultWorldGenerator(int seed): AbstractWorldGenerator(seed) {}

    void generateChunk(Chunk *chunk) override;
};

#endif //DEFAULTWORLDGENERATOR_H
