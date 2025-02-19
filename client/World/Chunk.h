#ifndef CHUNK_H
#define CHUNK_H

#include <iostream>
#include <glm/glm.hpp>

#include "Chunk.h"
#include "BakedChunk.h"
#include "Block.h"
#include "BlocksSource.h"
#include "../constants.h"
#include <array>

// TODO: Replace with real hash
static int fakeHashIndex = 0;

class Chunk {
public:
    explicit Chunk(Vec3i position): position(position) {
        this->hash = fakeHashIndex++;
        this->blocks = std::array<std::array<std::array<Block*, CHUNK_SIZE_XZ>, CHUNK_SIZE_Y>, CHUNK_SIZE_XZ>();

        for (auto blocks1 : blocks) {
            for (auto blocks2 : blocks1) {
                for (auto &block : blocks2) {
                    block = nullptr;
                }
            }
        }
    }

    int hash = -1;
    Vec3i position;

    std::array<std::array<std::array<Block*, CHUNK_SIZE_XZ>, CHUNK_SIZE_Y>, CHUNK_SIZE_XZ> blocks{};

    bool isNeedToUnload = false;

    void setBlock(BlockID id, Vec3i pos);

    [[nodiscard]] Block *getBlock(Vec3i pos) const;

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

    [[nodiscard]] bool isBaked() const;

    void addFace(std::vector<GLfloat> *vertices, std::vector<GLuint> *indices, Vec3i chunkPos, Block *currentBlock, glm::vec3 faceDirection, glm::vec3 offsets[], BlocksSource *blocksSource, Chunk *chunk);

    void bakeChunk(BlocksSource *blocksSource);

    [[nodiscard]] bool isBlockInBounds(Vec3i worldPos) const;

    Vec3i getBlockWorldPosition(Block *block) const;
};

#endif //CHUNK_H
