#include "Chunk.h"

#include "../constants.h"

void Chunk::setBlock(BlockID id, Vec3i pos) {
    assert(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
    assert(pos.x < CHUNK_SIZE_XZ);
    assert(pos.y < CHUNK_SIZE_Y);
    assert(pos.z < CHUNK_SIZE_XZ);
    assert(pos.x < this->blocks.size());
    assert(pos.x < this->blocks[pos.x].size());
    assert(pos.x < this->blocks[pos.x][pos.y].size());

    if (Block *block = this->blocks[pos.x][pos.y][pos.z]) block->setBlockId(id);
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
        if (anotherBlock && anotherBlock->getId() != 0 && anotherBlock->isSolid()) { // Find cover
            normalizedLight /= 1.2f; // Reduce light
        }
    }

    // Reduce light for sides
    if (faceDirection.y == 0) normalizedLight /= 2;

    float finalLight = normalizedLight;

    for (int i = 0; i < sizeof(localOffsets); i++) {
        glm::vec2 localOffset = localOffsets[i];
        glm::vec3 offset = offsets[i];

        vertices->push_back(static_cast<float>(relativePos.x) + (0.5f * offset.x));
        vertices->push_back(static_cast<float>(relativePos.y) + (0.5f * offset.y));
        vertices->push_back(static_cast<float>(relativePos.z) + (0.5f * offset.z));
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

    for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
                Block *currentBlock = this->getBlock(Vec3i(x, y, z));
                if (currentBlock == nullptr) continue;

                // Check each block's neighbors to determine which faces should be visible
                for (int i = 0; i < 6; ++i) {
                    // 6 faces per block
                    glm::vec3 faceDirection = faceDirections[i];

                    // Skip bottom face for bottom block
                    if (y == 0 && faceDirection.y == -1) continue;

                    std::vector<GLfloat> vertices;
                    std::vector<GLuint> indices;

                    // Check if the neighboring block exists or is air (to render the face)
                    Vec3i neighborWorldPos = this->getBlockWorldPosition(currentBlock) + neighborOffsets[i];
                    Block *neighborBlock = blocksSource->getBlock(neighborWorldPos);

                    if (neighborBlock == nullptr || (!neighborBlock->isSolid() && currentBlock->isSolid())) {
                        // Generate vertices and indices for the visible face
                        size_t vertexOffset = vertices.size() / 9;

                        if (faceDirection == glm::vec3(0, 0, -1)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(-1, -1, -1),
                                glm::vec3(1, -1, -1),
                                glm::vec3(1, 1, -1),
                                glm::vec3(-1, 1, -1)
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                        } else if (faceDirection == glm::vec3(0, 0, 1)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(-1, -1, 1),
                                glm::vec3(1, -1, 1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(-1, 1, 1)
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                        } else if (faceDirection == glm::vec3(0, -1, 0)) {
                           // if (y != 0) { // Do not place bottom face on bottom blocks
                                glm::vec3 offsets[] = {
                                    glm::vec3(-1, -1, -1),
                                    glm::vec3(1, -1, -1),
                                    glm::vec3(1, -1, 1),
                                    glm::vec3(-1, -1, 1),
                                };
                                addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                           // }
                        } else if (faceDirection == glm::vec3(0, 1, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(-1, 1, -1),
                                glm::vec3(1, 1, -1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(-1, 1, 1),
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                        } else if (faceDirection == glm::vec3(-1, 0, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(-1, -1, -1),
                                glm::vec3(-1, -1, 1),
                                glm::vec3(-1, 1, 1),
                                glm::vec3(-1, 1, -1),
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                        } else if (faceDirection == glm::vec3(1, 0, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(1, -1, -1),
                                glm::vec3(1, -1, 1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(1, 1, -1),
                            };
                            addFace(&vertices, &indices, this->position, currentBlock, faceDirection, offsets, blocksSource, this);
                        }

                        indices.push_back(vertexOffset + 0);
                        indices.push_back(vertexOffset + 1);
                        indices.push_back(vertexOffset + 2);
                        indices.push_back(vertexOffset + 2);
                        indices.push_back(vertexOffset + 3);
                        indices.push_back(vertexOffset + 0);

                    }

                    // If there are vertices and indices for this face, create a part and add it to the baked chunk
                    if (!vertices.empty() && !indices.empty()) {
                        BakedChunkPart part;
                        part.vertices = std::move(vertices);
                        part.indices = std::move(indices);
                        part.blockID = currentBlock->getId();
                        part.isSolid = currentBlock->isSolid();
                        part.isBuffered = false;

                        if (part.isSolid)
                            bakedChunk->chunkParts.push_back(part);
                        else
                            bakedChunk->liquidChunkParts.push_back(part);
                    }
                }
            }
        }
    }

    long endMs = SDL_GetTicks();
    long diffMs = endMs - startMs;
    std::cout << "Baked chunk #" << this->hash << " in " << diffMs << " ms" << std::endl;

    this->bakedChunk = bakedChunk;
    this->hash = fakeHashIndex++;
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