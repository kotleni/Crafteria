#include "Chunk.h"

#include <unordered_map>

#include "../constants.h"

void Chunk::setBlock(BlockID id, Vec3i pos) {
    assert(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
    assert(pos.x < CHUNK_SIZE_XZ);
    assert(pos.y < CHUNK_SIZE_Y);
    assert(pos.z < CHUNK_SIZE_XZ);
    assert(pos.x < this->blocks.size());
    assert(pos.x < this->blocks[pos.x].size());
    assert(pos.x < this->blocks[pos.x][pos.y].size());

    if (Block *block = this->blocks[pos.x][pos.y][pos.z])
            block->setBlockId(id);
    else this->blocks[pos.x][pos.y][pos.z] = new Block(pos, id);
}

Block *Chunk::getBlock(Vec3i pos) const {
    // assert(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
    // assert(pos.x < CHUNK_SIZE_XZ);
    // assert(pos.y < CHUNK_SIZE_Y);
    // assert(pos.z < CHUNK_SIZE_XZ);
    // assert(pos.x < this->blocks.size());
    // assert(pos.x < this->blocks[pos.x].size());
    // assert(pos.x < this->blocks[pos.x][pos.y].size());

    if (pos.x < 0 || pos.y < 0 || pos.z < 0 ||
        pos.x >= CHUNK_SIZE_XZ || pos.y >= CHUNK_SIZE_Y || pos.z >= CHUNK_SIZE_XZ)
        return nullptr;

    return this->blocks[pos.x][pos.y][pos.z];
}

bool Chunk::isBaked() const {
    return bakedChunk != nullptr;
}

void Chunk::addFace(std::vector<GLfloat> *vertices, std::vector<GLuint> *indices, Vec3i chunkPos, Block *currentBlock,
                    glm::vec3 faceDirection,
                    glm::vec3 offsets[],
                    BlocksSource *blocksSource,
                    Chunk *chunk) {

    glm::vec2 localOffsets[] = {
        glm::vec2(0, 0),
        glm::vec2(1, 0),
        glm::vec2(1, 1),
        glm::vec2(0, 1)
    };

    // Vec3i worldPos = chunkPos * CHUNK_SIZE_XZ;

    // TODO: Replace foreach with more quick way
    const BlockData *blockData;
    for (const BlockData& _blockData: BLOCKS_DATA) {
        if (_blockData.blockID == currentBlock->getId()) {
            blockData = &_blockData;
            break;
        }
    }

    if (!blockData) {
        std::cout << "Can't find block data for " << currentBlock->getId() << std::endl;
        return;
    }

    Vec3i relativePos = currentBlock->getChunkPosition();
    //     {
    //     currentBlock->position.x, //- worldPos.x,
    //     currentBlock->position.y, //- worldPos.y,
    //     currentBlock->position.z //- worldPos.z
    // };

    float normalizedLight = 1.0f;

    // TODO: Move it from addFace to function what call it
    Vec3i blockPos = this->getBlockWorldPosition(currentBlock);
    // Check if any blocks on top cover this block
    for (int y = CHUNK_SIZE_Y - 1; y > blockPos.y; y--) {
        auto anotherBlock = blocksSource->getBlock(Vec3i(blockPos.x, y, blockPos.z));
        if (anotherBlock && anotherBlock->getId() != 0 && anotherBlock->isSolid() && !anotherBlock->isFlora()) { // Find cover
            normalizedLight /= 1.2f; // Reduce light
        }
    }

    // Reduce light for sides
    if (faceDirection.y == 0) normalizedLight /= 2;

    // Torch light influence
    const int TORCH_RADIUS = 5;
    float torchLight = 0.0f;

    for (int dx = -TORCH_RADIUS; dx <= TORCH_RADIUS; dx++) {
        for (int dy = -TORCH_RADIUS; dy <= TORCH_RADIUS; dy++) {
            for (int dz = -TORCH_RADIUS; dz <= TORCH_RADIUS; dz++) {
                Vec3i nearbyPos = blockPos + Vec3i(dx, dy, dz);
                auto nearbyBlock = blocksSource->getBlock(nearbyPos);
                if (nearbyBlock && nearbyBlock->getId() == BLOCK_TORCH) {
                    float distance = glm::length(glm::vec3(dx, dy, dz));
                    torchLight += std::max(0.0f, 0.5f - (distance / TORCH_RADIUS));
                }
            }
        }
    }

    float finalLight = std::min(1.0f, normalizedLight + torchLight);
    // float finalLight = normalizedLight;

    for (int i = 0; i < sizeof(localOffsets); i++) {
        glm::vec2 localOffset = localOffsets[i];
        glm::vec3 offset = offsets[i];

        vertices->push_back(static_cast<float>(relativePos.x) + offset.x);
        vertices->push_back(static_cast<float>(relativePos.y) + offset.y);
        vertices->push_back(static_cast<float>(relativePos.z) + offset.z);
        vertices->push_back(faceDirection.x);
        vertices->push_back(faceDirection.y);
        vertices->push_back(faceDirection.z);
        vertices->push_back(localOffset.x);
        vertices->push_back(localOffset.y);

        vertices->push_back(finalLight);
    }
}

void Chunk::bakeChunk(BlocksSource *blocksSource) {
    long startMs = SDL_GetTicks();

    auto bakedChunk = new BakedChunk();

    std::pmr::unordered_map<BlockID, std::vector<GLfloat>> verticesMap;
    std::pmr::unordered_map<BlockID, std::vector<GLuint>> indicesMap;

    for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
                Block *currentBlock = this->getBlock(Vec3i(x, y, z));
                if (currentBlock == nullptr || currentBlock->getId() == BLOCK_AIR) continue;

                std::vector<GLfloat> vertices = verticesMap[currentBlock->getId()];
                std::vector<GLuint> indices = indicesMap[currentBlock->getId()];

                // Check each block's neighbors to determine which faces should be visible
                for (int i = 0; i < 6; ++i) {
                    // 6 faces per block
                    glm::vec3 faceDirection = faceDirections[i];

                    // Skip bottom face for bottom block
                    if (y == 0 && faceDirection.y == -1) continue;

                    // Check if the neighboring block exists or is air (to render the face)
                    Vec3i neighborWorldPos = this->getBlockWorldPosition(currentBlock) + neighborOffsets[i];
                    Block *neighborBlock = blocksSource->getBlock(neighborWorldPos);

                    size_t vertexOffset = vertices.size() / 9;
                    GLuint indicesFront[] = {
                        static_cast<GLuint>(vertexOffset + 0),
                        static_cast<GLuint>(vertexOffset + 1),
                        static_cast<GLuint>(vertexOffset + 2),
                        static_cast<GLuint>(vertexOffset + 2),
                        static_cast<GLuint>(vertexOffset + 3),
                        static_cast<GLuint>(vertexOffset + 0),
                     };

                    GLuint indicesBack[] = {
                        static_cast<GLuint>(vertexOffset + 2),
                        static_cast<GLuint>(vertexOffset + 1),
                        static_cast<GLuint>(vertexOffset + 0),
                        static_cast<GLuint>(vertexOffset + 0),
                        static_cast<GLuint>(vertexOffset + 3),
                        static_cast<GLuint>(vertexOffset + 2),
                     };

                    if (currentBlock->isFlora()) { // Flora has different geometry
                        GLuint vertexOffset = vertices.size() / 9;

                        // First quad (diagonal in XZ plane, centered inside the block)
                        glm::vec3 offsets1[] = {
                            glm::vec3(0.0f, 0.0f, 0.0f),
                            glm::vec3(1.0f, 0.0f, 1.0f),
                            glm::vec3(1.0f, 1.0f, 1.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f)
                        };

                        addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets1, blocksSource, this);

                        // Front face
                        GLuint indices1Front[] = {
                            vertexOffset + 0, vertexOffset + 1, vertexOffset + 2,
                            vertexOffset + 2, vertexOffset + 3, vertexOffset + 0
                        };
                        indices.insert(indices.end(), std::begin(indices1Front), std::end(indices1Front));
                        vertexOffset = vertices.size() / 9;

                        // Back face (reversed order)
                        GLuint indices1Back[] = {
                            vertexOffset + 2, vertexOffset + 1, vertexOffset + 0,
                            vertexOffset + 0, vertexOffset + 3, vertexOffset + 2
                        };
                        indices.insert(indices.end(), std::begin(indices1Back), std::end(indices1Back));
                        vertexOffset = vertices.size() / 9;

                        // Second quad (rotated 90°, also centered in the block)
                        glm::vec3 offsets2[] = {
                            glm::vec3(1.0f, 0.0f, 0.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f),
                            glm::vec3(0.0f, 1.0f, 1.0f),
                            glm::vec3(1.0f, 1.0f, 0.0f)
                        };

                        addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets2, blocksSource, this);

                        // Front face
                        GLuint indices2Front[] = {
                            vertexOffset + 0, vertexOffset + 1, vertexOffset + 2,
                            vertexOffset + 2, vertexOffset + 3, vertexOffset + 0
                        };
                        indices.insert(indices.end(), std::begin(indices2Front), std::end(indices2Front));
                        vertexOffset = vertices.size() / 9;

                        // Back face (reversed order)
                        GLuint indices2Back[] = {
                            vertexOffset + 2, vertexOffset + 1, vertexOffset + 0,
                            vertexOffset + 0, vertexOffset + 3, vertexOffset + 2
                        };
                        indices.insert(indices.end(), std::begin(indices2Back), std::end(indices2Back));
                    }
                    else if (neighborBlock == nullptr || ((!neighborBlock->isSolid() || neighborBlock->isFlora()) && currentBlock->isSolid())) {
                        if (faceDirection == glm::vec3(0, 0, -1)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(0, 0, 0),
                                glm::vec3(1, 0, 0),
                                glm::vec3(1, 1, 0),
                                glm::vec3(0, 1, 0)
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                            indices.insert(indices.end(), std::begin(indicesBack), std::end(indicesBack));
                        } else if (faceDirection == glm::vec3(0, 0, 1)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(0, 0, 1),
                                glm::vec3(1, 0, 1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(0, 1, 1)
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                            indices.insert(indices.end(), std::begin(indicesFront), std::end(indicesFront));
                        } else if (faceDirection == glm::vec3(0, -1, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(0, 0, 0),
                                glm::vec3(1, 0, 0),
                                glm::vec3(1, 0, 1),
                                glm::vec3(0, 0, 1),
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                            indices.insert(indices.end(), std::begin(indicesFront), std::end(indicesFront));
                        } else if (faceDirection == glm::vec3(0, 1, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(0, 1, 0),
                                glm::vec3(1, 1, 0),
                                glm::vec3(1, 1, 1),
                                glm::vec3(0, 1, 1),
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                            indices.insert(indices.end(), std::begin(indicesBack), std::end(indicesBack));
                        } else if (faceDirection == glm::vec3(-1, 0, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(0, 0, 0),
                                glm::vec3(0, 0, 1),
                                glm::vec3(0, 1, 1),
                                glm::vec3(0, 1, 0),
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                            indices.insert(indices.end(), std::begin(indicesFront), std::end(indicesFront));
                        } else if (faceDirection == glm::vec3(1, 0, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(1, 0, 0),
                                glm::vec3(1, 0, 1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(1, 1, 0),
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                            indices.insert(indices.end(), std::begin(indicesBack), std::end(indicesBack));
                        }
                    }
                }

                verticesMap[currentBlock->getId()] = vertices;
                indicesMap[currentBlock->getId()] = indices;
            }
        }
    }

    // For each block
    // Create separated chunk part
    for (const BlockData& blockData: BLOCKS_DATA) {
        auto vertices = verticesMap[blockData.blockID];
        auto indices = indicesMap[blockData.blockID];

        // Skip emptys
        if (vertices.empty() || indices.empty()) continue;

        BakedChunkPart part;
        part.vertices = std::move(vertices);
        part.indices = std::move(indices);
        part.blockID = blockData.blockID;
        part.isSolid = blockData.isSolid;
        part.isFlora = blockData.isFlora;
        part.isBuffered = false;

        if (part.isSolid)
            bakedChunk->chunkParts.push_back(part);
        else if (part.isFlora)
            bakedChunk->floraChunkParts.push_back(part);
        else
            bakedChunk->liquidChunkParts.push_back(part);
    }

    long endMs = SDL_GetTicks();
    long diffMs = endMs - startMs;
    std::cout << "Baked chunk #" << this->hash << " in " << diffMs << " ms" << std::endl;

    if (!this->bakedChunk) { // If first baking - just save it
        this->bakedChunk = bakedChunk;
    } else { // If it's requested update - save as next
        this->nextBakedChunk = bakedChunk;
    }

    this->hash = fakeHashIndex++;
    this->isNeedToRebake = false;
}

bool Chunk::isBlockInBounds(Vec3i worldPos) const {
    return (worldPos.x >= position.x * CHUNK_SIZE_XZ && worldPos.x < (position.x + 1) * CHUNK_SIZE_XZ &&
            worldPos.z >= position.z * CHUNK_SIZE_XZ && worldPos.z < (position.z + 1) * CHUNK_SIZE_XZ);
}

Vec3i Chunk::getBlockWorldPosition(Block *block) const {
    Vec3i chunkWorldPos = this->position;
    chunkWorldPos.x *= CHUNK_SIZE_XZ;
    chunkWorldPos.z *= CHUNK_SIZE_XZ;
    return chunkWorldPos + block->getChunkPosition();
}

void Chunk::requestRebake() {
    this->isNeedToRebake = true;
}