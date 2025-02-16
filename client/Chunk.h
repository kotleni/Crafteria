#ifndef CHUNK_H
#define CHUNK_H

#include <iostream>
#include <glm/glm.hpp>

#include "BakedChunk.h"
#include "Block.h"


// TODO: Replace with real hash
static int fakeHashIndex = 0;

class Chunk {
public:
    Chunk(Vec3i position): position(position) {
        this->hash = fakeHashIndex++;
    }

    int hash = -1;
    Vec3i position;

    std::vector<Block *> blocks;

    void setBlock(BlockID id, Vec3i pos);

    Block *getBlock(Vec3i pos);

    BakedChunk *bakedChunk = nullptr;
    //std::pmr::unordered_map<int, BakedChunk *> cachedBakedChunks;

    Vec3i neighborOffsets[6] = {
        Vec3i(0, 0, -1), // front
        Vec3i(0, 0, 1), // back
        Vec3i(0, -1, 0), // bottom
        Vec3i(0, 1, 0), // top
        Vec3i(-1, 0, 0), // left
        Vec3i(1, 0, 0), // right
    };

    glm::vec3 faceDirections[6] = {
        glm::vec3(0, 0, -1), // front
        glm::vec3(0, 0, 1), // back
        glm::vec3(0, -1, 0), // bottom
        glm::vec3(0, 1, 0), // top
        glm::vec3(-1, 0, 0), // left
        glm::vec3(1, 0, 0), // right
    };

    bool isBaked();

    void addFace(std::vector<GLfloat> *vertices, std::vector<GLuint> *indices, Block *currentBlock, glm::vec3 faceDirection, glm::vec3 offsets[]);

    void bakeChunk();
};

#endif //CHUNK_H
