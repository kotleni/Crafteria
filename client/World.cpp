#include "World.h"

World::World(int seedValue) {
    this->player = new Player();
    this->seedValue = seedValue;
    this->perlin = siv::PerlinNoise(seedValue);

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
    for (Block *block: chunk->blocks) {
        delete block;
    }
    delete chunk->bakedChunk;
    this->chunks.erase(std::remove(this->chunks.begin(), this->chunks.end(), chunk), this->chunks.end());
    std::cout << "Chunk " << chunk->hash << " unloaded" << std::endl;
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

            if (!chunk->isBaked()) {
                chunk->bakeChunk(this);
            }
        }
    }
}

void World::generateFilledChunk(Vec3i pos) {
    auto *chunk = new Chunk(pos);
    chunks.push_back(chunk);

    constexpr float scale = 0.08f;
    constexpr float heightMultiplier = 6.0f;
    constexpr int octaves = 5;
    constexpr int seaLevel = 12;
    constexpr int realSeaLevel = 15;

    int baseX = pos.x * CHUNK_SIZE_XYZ;
    int baseZ = pos.z * CHUNK_SIZE_XYZ;

    for (int x = 0; x < CHUNK_SIZE_XYZ; ++x) {
        for (int z = 0; z < CHUNK_SIZE_XYZ; ++z) {
            double yMod = perlin.octave2D_01((baseX + x) * scale, (baseZ + z) * scale, octaves);
            int y = static_cast<int>(yMod * heightMultiplier) + seaLevel;
            int xx = baseX + x;
            int zz = baseZ + z;

            double temperature = perlin.octave2D_11((baseZ + x) * scale * 0.5, (baseX + z) * scale * 0.5, octaves);

            BlockID surfaceBlock;
            if (temperature > 0.4) {
                surfaceBlock = BLOCK_GRASS;
            } else if (temperature > -0.2) {
                surfaceBlock = BLOCK_SAND;
            } else {
                surfaceBlock = BLOCK_COBBLESTONE;
            }

            chunk->setBlock(surfaceBlock, {xx, y, zz});

            for (int depth = 1; depth < 16; ++depth) {
                int depthY = y - depth;
                if (depthY < 0) break; // Avoid out-of-bounds

                if (depth < 3) {
                    chunk->setBlock(BLOCK_DIRT, {xx, depthY, zz});
                } else {
                    chunk->setBlock(BLOCK_COBBLESTONE, {xx, depthY, zz});
                }
            }

            // Add water if below sea level
            if (y < realSeaLevel) {
                for (int waterY = y; waterY <= realSeaLevel; ++waterY) {
                    Vec3i pos = {xx, waterY, zz};
                    if (chunk->getBlock(pos) == nullptr)
                        chunk->setBlock(BLOCK_WATER, pos); // Water block
                }
            }
        }
    }
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