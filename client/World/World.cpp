#include "World.h"

World::World(int seedValue, RuntimeConfig *runtimeConfig) {
    this->player = new Player();
    this->seedValue = seedValue;
    this->runtimeConfig = runtimeConfig;
    this->generator = new DefaultWorldGenerator(seedValue);

    for (int i = 0; i < BAKING_CHUNK_THREADS_LIMIT; ++i) {
        threads.emplace_back(&World::updateChunks, this);
    }
}

bool World::isChunkExist(Vec3i chunkPos) {
    for (Chunk *chunk: this->chunks) {
        if (chunk->position == chunkPos) {
            return true;
        }
    }
    return false;
}

void World::markChunkToUnload(Chunk *chunk) {
    chunk->isNeedToUnload = true;
}

// TODO: Review all allocable things
void World::unloadChunk(Chunk *chunk) {
    int hash = chunk->hash;

    delete chunk;
    std::erase(this->chunks, chunk);
    std::cout << "Chunk #" << hash << " unloaded" << std::endl;
}

bool World::areNeighborsGenerated(const Vec3i &chunkPos) {
    static const std::vector<Vec3i> neighborOffsets = {
        {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}
    };

    for (const auto &offset : neighborOffsets) {
        Vec3i neighborPos = chunkPos + offset;
        if (!isChunkExist(neighborPos)) {
            return false; // A neighbor is missing
        }
    }
    return true;
}

void World::updateChunks() {
    while (true) {
        // Vec3i playerPos = {static_cast<int>(this->player->position.x), static_cast<int>(this->player->position.y), static_cast<int>(this->player->position.z)};
        Vec3i playerChunkPos = {
            static_cast<int>(this->player->getPosition().x / CHUNK_SIZE_XZ), 0,
            static_cast<int>(this->player->getPosition().z / CHUNK_SIZE_XZ)
        };

        // Find not generated chunks around player
        auto maxDistance = runtimeConfig->maxRenderingDistance;
        bool isFound = false;
        Vec3i targetChunkPos = {0, 0, 0};
        double targetChunkDistance = 99999;
        for (int x = -maxDistance; x < maxDistance; x++) {
            for (int z = -maxDistance; z < maxDistance; z++) {
                Vec3i chunkPos = playerChunkPos + Vec3i(x, 0, z);
                if (!isChunkExist(chunkPos)) {
                    // generateFilledChunk(chunkPos);

                    auto distance = playerChunkPos.distanceTo(chunkPos);
                    if (distance < targetChunkDistance && distance < maxDistance) {
                        targetChunkDistance = distance;
                        targetChunkPos = chunkPos;
                        isFound = true;
                    }
                }
            }
        }

        if (isFound && this-runtimeConfig->isChunkGenerationEnabled) {
            generateFilledChunk(targetChunkPos);
        }

        for (Chunk *chunk: chunks) {
            auto chunkPos = chunk->position;
            auto distance = round(playerChunkPos.distanceTo(chunkPos));
            if (distance > runtimeConfig->maxRenderingDistance) {
                markChunkToUnload(chunk);
            }

            if ((!chunk->isBaked() || chunk->isNeedToRebake) && areNeighborsGenerated(chunkPos) && !chunk->isNeedToUnload && this-runtimeConfig->isChunkBakingEnabled) {
                chunk->bakeChunk(this);
            }
        }
    }
}

void World::generateFilledChunk(Vec3i pos) {
    auto *chunk = new Chunk(pos);
    this->generator->generateChunk(chunk);
    chunks.push_back(chunk);
}


Block *World::getBlock(Vec3i worldPos) {
    // TODO: Calculate chunk pos instead of searching
    for (Chunk *chunk : this->chunks) {
        if (chunk->isBlockInBounds(worldPos)) {
            Block *block = chunk->getBlock(worldPos - (chunk->position * CHUNK_SIZE_XZ));
            if (block != nullptr)
                return block;
        }
    }
    return nullptr;
}

Chunk* World::findChunkByChunkPos(Vec3i pos) {
    for (Chunk *chunk: this->chunks) {
        if (chunk->position == pos)
            return chunk;
    }

    return nullptr;
}

void World::setBlock(BlockID id, Vec3i worldPos) {
    // TODO: Calculate chunk pos instead of searching
    for (Chunk *chunk : this->chunks) {
        if (chunk->isBlockInBounds(worldPos)) {
            Vec3i blockInChunkPos = worldPos - (chunk->position * CHUNK_SIZE_XZ);
            chunk->setBlock(id, blockInChunkPos);
            chunk->requestRebake();

            // Update neighbors chunk
            Vec3i chunkPos = chunk->position;
            if (blockInChunkPos.x == 0) {
                chunkPos = chunkPos + Vec3i(-1, 0, 0);
            } else if (blockInChunkPos.x == CHUNK_SIZE_XZ - 1) {
                chunkPos = chunkPos + Vec3i(1, 0, 0);
            } else if (blockInChunkPos.z == 0) {
                chunkPos = chunkPos + Vec3i(0, 0, -1);
            } else if (blockInChunkPos.z == CHUNK_SIZE_XZ - 1) {
                chunkPos = chunkPos + Vec3i(0, 0, 1);
            }

            if (chunkPos != chunk->position) {
                if (Chunk *chunk = findChunkByChunkPos(chunkPos)) {
                    chunk->requestRebake();
                }
            }
            return;
        }
    }
}