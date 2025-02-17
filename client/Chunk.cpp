#include "Chunk.h"

#include "constants.h"

void Chunk::setBlock(BlockID id, Vec3i pos) {
    for (Block *cube: blocks) {
        if (cube->position == pos) {
            cube->id = id; // Replace id
            return;
        }
    }

    blocks.push_back(new Block(pos, id));
}

Block *Chunk::getBlock(Vec3i pos) {
    for (Block *cube: blocks) {
        if (cube->position == pos) {
            return cube;
        }
    }
    return nullptr;
}

bool Chunk::isBaked() {
    if (bakedChunk == nullptr) return false;
    if (bakedChunk->isOutdated()) return false;
    return true;
}

void Chunk::addFace(std::vector<GLfloat> *vertices, std::vector<GLuint> *indices, Vec3i chunkPos, Block *currentBlock,
                    glm::vec3 faceDirection,
                    glm::vec3 offsets[]) {
    glm::vec2 localOffsets[] = {
        glm::vec2(0, 0),
        glm::vec2(1, 0),
        glm::vec2(1, 1),
        glm::vec2(0, 1)
    };

    Vec3i worldPos = chunkPos * CHUNK_SIZE_XYZ;

    Vec3i relativePos = {
        currentBlock->position.x - worldPos.x,
        currentBlock->position.y - worldPos.y,
        currentBlock->position.z - worldPos.z
    };

    for (int i = 0; i < sizeof(localOffsets); i++) {
        glm::vec2 localOffset = localOffsets[i];
        glm::vec3 offset = offsets[i];

        vertices->push_back(relativePos.x + (0.5f * offset.x));
        vertices->push_back(relativePos.y + (0.5f * offset.y));
        vertices->push_back(relativePos.z + (0.5f * offset.z));
        vertices->push_back(faceDirection.x);
        vertices->push_back(faceDirection.y);
        vertices->push_back(faceDirection.z);
        vertices->push_back(localOffset.x);
        vertices->push_back(localOffset.y);
    }
}

void Chunk::bakeChunk(BlocksSource *blocksSource) {
    Chunk *chunk = this;
    auto bakedChunk = new BakedChunk();

    for (const auto &block: chunk->blocks) {
        Block *currentBlock = block;
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
                int vertexOffset = vertices.size() / 8;

                if (faceDirection == glm::vec3(0, 0, -1)) {
                    glm::vec3 offsets[] = {
                        glm::vec3(-1, -1, -1),
                        glm::vec3(1, -1, -1),
                        glm::vec3(1, 1, -1),
                        glm::vec3(-1, 1, -1)
                    };
                    addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets);
                } else if (faceDirection == glm::vec3(0, 0, 1)) {
                    glm::vec3 offsets[] = {
                        glm::vec3(-1, -1, 1),
                        glm::vec3(1, -1, 1),
                        glm::vec3(1, 1, 1),
                        glm::vec3(-1, 1, 1)
                    };
                    addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets);
                } else if (faceDirection == glm::vec3(0, -1, 0)) {
                    glm::vec3 offsets[] = {
                        glm::vec3(-1, -1, -1),
                        glm::vec3(1, -1, -1),
                        glm::vec3(1, -1, 1),
                        glm::vec3(-1, -1, 1),
                    };
                    addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets);
                } else if (faceDirection == glm::vec3(0, 1, 0)) {
                    glm::vec3 offsets[] = {
                        glm::vec3(-1, 1, -1),
                        glm::vec3(1, 1, -1),
                        glm::vec3(1, 1, 1),
                        glm::vec3(-1, 1, 1),
                    };
                    addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets);
                } else if (faceDirection == glm::vec3(-1, 0, 0)) {
                    glm::vec3 offsets[] = {
                        glm::vec3(-1, -1, -1),
                        glm::vec3(-1, -1, 1),
                        glm::vec3(-1, 1, 1),
                        glm::vec3(-1, 1, -1),
                    };
                    addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets);
                } else if (faceDirection == glm::vec3(1, 0, 0)) {
                    glm::vec3 offsets[] = {
                        glm::vec3(1, -1, -1),
                        glm::vec3(1, -1, 1),
                        glm::vec3(1, 1, 1),
                        glm::vec3(1, 1, -1),
                    };
                    addFace(&vertices, &indices, chunk->position, currentBlock, faceDirection, offsets);
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

    std::cout << "Baked new chunk " << chunk->hash << std::endl;

    this->bakedChunk = bakedChunk;
    this->hash = fakeHashIndex++;
}

bool Chunk::isBlockInBounds(Vec3i worldPos) {
    return (worldPos.x >= position.x * CHUNK_SIZE_XYZ && worldPos.x < (position.x + 1) * CHUNK_SIZE_XYZ &&
            worldPos.z >= position.z * CHUNK_SIZE_XYZ && worldPos.z < (position.z + 1) * CHUNK_SIZE_XYZ);
}