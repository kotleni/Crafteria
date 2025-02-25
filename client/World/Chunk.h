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
private:
    BakedChunk *bakedChunk = nullptr;
    BakedChunk *nextBakedChunk = nullptr;
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

    ~Chunk() {
        for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
            for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
                for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
                    Block *block = this->blocks[x][y][z];
                    delete block;
                }
            }
        }
        delete this->bakedChunk;
    }

    int hash = -1;
    Vec3i position;

    std::array<std::array<std::array<Block*, CHUNK_SIZE_XZ>, CHUNK_SIZE_Y>, CHUNK_SIZE_XZ> blocks{};

    bool isNeedToUnload = false;
    bool isNeedToRebake = false;

    void setBlock(BlockID id, Vec3i pos);

    [[nodiscard]] Block *getBlock(Vec3i pos) const;
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
    void requestRebake();

    [[nodiscard]] bool isBlockInBounds(Vec3i worldPos) const;

    Vec3i getBlockWorldPosition(Block *block) const;

    BakedChunk *getBakedChunk() {
        if (this->nextBakedChunk) {
            delete this->bakedChunk;
            this->bakedChunk = this->nextBakedChunk;
            this->nextBakedChunk = nullptr;
        }
        return this->bakedChunk;
    }
};

#endif //CHUNK_H
