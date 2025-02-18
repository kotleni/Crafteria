#include "Chunk.h"

#include "../constants.h"

void Chunk::setBlock(BlockID id, Vec3i pos) {
    assert(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
    assert(pos.x < CHUNK_SIZE_XZ);
    assert(pos.y < CHUNK_SIZE_Y);
    assert(pos.z < CHUNK_SIZE_XZ);

    if (Block *block = this->blocks[pos.x][pos.y][pos.z]) block->id = id;
    else this->blocks[pos.x][pos.y][pos.z] = new Block(pos, id);
}

Block *Chunk::getBlock(Vec3i pos) const {
    return this->blocks[pos.x][pos.y][pos.z];
}

bool Chunk::isBaked() const {
    return bakedChunk != nullptr;
}

void Chunk::addFace(std::vector<GLfloat> *vertices, std::vector<GLuint> *indices, Vec3i chunkPos, const Block *currentBlock,
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

    Vec3i worldPos = chunkPos * CHUNK_SIZE_XZ;

    Vec3i relativePos = {
        currentBlock->position.x - worldPos.x,
        currentBlock->position.y - worldPos.y,
        currentBlock->position.z - worldPos.z
    };

    float normalizedLight = 1.0f;

    // Reduce light for under solid blocks
    auto topBlock = blocksSource->getBlock(currentBlock->position + Vec3i(0, 1, 0));
    if (topBlock && !topBlock->isSolid()) normalizedLight /= 4;
    // Reduce light for sides
    else if (faceDirection.y == 0) normalizedLight /= 2;

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

    Chunk *chunk = this;

    auto bakedChunk = new BakedChunk();

    for (int x = 0; x < CHUNK_SIZE_XZ; ++x) {
        for (int y = 0; y < CHUNK_SIZE_Y; ++y) {
            for (int z = 0; z < CHUNK_SIZE_XZ; ++z) {
                Block *currentBlock = chunk->getBlock(Vec3i(x, y, z));
                if (currentBlock == nullptr) continue;

                // Check each block's neighbors to determine which faces should be visible
                for (int i = 0; i < 6; ++i) {
                    // 6 faces per block
                    glm::vec3 faceDirection = faceDirections[i];
                    std::vector<GLfloat> vertices;
                    std::vector<GLuint> indices;

                    // Check if the neighboring block exists or is air (to render the face)
                    Vec3i neighborWorldPos = currentBlock->position + neighborOffsets[i];
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
                            addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets, blocksSource, chunk);
                        } else if (faceDirection == glm::vec3(0, 0, 1)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(-1, -1, 1),
                                glm::vec3(1, -1, 1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(-1, 1, 1)
                            };
                            addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets, blocksSource, chunk);
                        } else if (faceDirection == glm::vec3(0, -1, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(-1, -1, -1),
                                glm::vec3(1, -1, -1),
                                glm::vec3(1, -1, 1),
                                glm::vec3(-1, -1, 1),
                            };
                            addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets, blocksSource, chunk);
                        } else if (faceDirection == glm::vec3(0, 1, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(-1, 1, -1),
                                glm::vec3(1, 1, -1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(-1, 1, 1),
                            };
                            addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets, blocksSource, chunk);
                        } else if (faceDirection == glm::vec3(-1, 0, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(-1, -1, -1),
                                glm::vec3(-1, -1, 1),
                                glm::vec3(-1, 1, 1),
                                glm::vec3(-1, 1, -1),
                            };
                            addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets, blocksSource, chunk);
                        } else if (faceDirection == glm::vec3(1, 0, 0)) {
                            glm::vec3 offsets[] = {
                                glm::vec3(1, -1, -1),
                                glm::vec3(1, -1, 1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(1, 1, -1),
                            };
                            addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets, blocksSource, chunk);
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
                        part.blockID = currentBlock->id;
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
    std::cout << "Baked chunk #" << chunk->hash << " in " << diffMs << " ms" << std::endl;

    this->bakedChunk = bakedChunk;
    this->hash = fakeHashIndex++;
}

bool Chunk::isBlockInBounds(Vec3i worldPos) const {
    return (worldPos.x >= position.x * CHUNK_SIZE_XZ && worldPos.x < (position.x + 1) * CHUNK_SIZE_XZ &&
            worldPos.z >= position.z * CHUNK_SIZE_XZ && worldPos.z < (position.z + 1) * CHUNK_SIZE_XZ);
}