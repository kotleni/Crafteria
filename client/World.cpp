#include "World.h"

World::World(int seedValue) {
    this->player = new Player();
    this->seedValue = seedValue;
    this->generator = new DefaultWorldGenerator(seedValue);

    for (int i = 0; i < BAKING_CHUNK_THREADS_LIMIT; ++i) {
        threads.emplace_back(&World:: updateChunks, this);
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
    for (Block *block: chunk->blocks) {
        delete block;
    }
    delete chunk->bakedChunk;
    this->chunks.erase(std::remove(this->chunks.begin(), this->chunks.end(), chunk), this->chunks.end());
    std::cout << "Chunk " << chunk->hash << " unloaded" << std::endl;
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
            static_cast<int>(this->player->position.x / CHUNK_SIZE_XYZ), 0,
            static_cast<int>(this->player->position.z / CHUNK_SIZE_XYZ)
        };

        // Find not generated chunks around player
        auto maxDistance = CHUNK_RENDERING_DISTANCE;
        bool isFound = false;
        Vec3i targetChunkPos = {0, 0, 0};
        double targetChunkDistance = 99999;
        for (int x = -maxDistance; x < maxDistance; x++) {
            for (int z = -maxDistance; z < maxDistance; z++) {
                Vec3i chunkPos = playerChunkPos + Vec3i(x, 0, z);
                if (!isChunkExist(chunkPos)) {
                    // generateFilledChunk(chunkPos);

                    auto distance = playerChunkPos.distanceTo(chunkPos);
                    if (distance < targetChunkDistance) {
                        targetChunkDistance = distance;
                        targetChunkPos = chunkPos;
                        isFound = true;
                    }
                }
            }
        }

        if (isFound) {
            generateFilledChunk(targetChunkPos);
        }

        for (Chunk *chunk: chunks) {
            auto chunkPos = chunk->position;
            auto distance = playerChunkPos.distanceTo(chunkPos);
            if (distance > CHUNK_RENDERING_DISTANCE) {
                markChunkToUnload(chunk);
            }

            if (!chunk->isBaked() && areNeighborsGenerated(chunkPos)) {
                chunk->bakeChunk(this);
            }
        }
    }
}

void World::generateFilledChunk(Vec3i pos) {
    auto *chunk = new Chunk(pos);
    chunks.push_back(chunk);

    this->generator->generateChunk(chunk);
}


Block *World::getBlock(Vec3i worldPos) {
    for (Chunk *chunk : this->chunks) {
        if (chunk->isBlockInBounds(worldPos)) {
            Block *block = chunk->getBlock(worldPos);
            if (block != nullptr)
                return block;
        }
    }
    return nullptr;
}

void World::setBlock(BlockID id, Vec3i worldPos) {
    for (Chunk *chunk : this->chunks) {
        if (chunk->isBlockInBounds(worldPos)) {
            chunk->setBlock(id, worldPos);
            return;
        }
    }
}