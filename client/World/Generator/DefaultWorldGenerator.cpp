#include "DefaultWorldGenerator.h"

void DefaultWorldGenerator::generateChunk(Chunk *chunk) {
    constexpr float scale = 0.08f;
    constexpr float heightMultiplier = 8.0f;
    constexpr int octaves = 5;
    constexpr int seaLevel = 64;
    constexpr int realSeaLevel = 67;

    int chunkWorldX = chunk->position.x * CHUNK_SIZE_XZ;
    int chunkWorldZ = chunk->position.z * CHUNK_SIZE_XZ;

    for (int xx = 0; xx < CHUNK_SIZE_XZ; ++xx) {
        for (int zz = 0; zz < CHUNK_SIZE_XZ; ++zz) {
            int worldX = chunkWorldX + xx;
            int worldZ = chunkWorldZ + zz;

            double yMod = this->perlin.octave2D_01(worldX * scale, worldZ * scale, octaves);
            int y = static_cast<int>(yMod * heightMultiplier) + seaLevel;

            assert(y >= 0 && y < CHUNK_SIZE_Y);

            double temperature = this->perlin.octave2D_11(worldX * scale * 0.5, worldZ * scale * 0.5, octaves);
            double populationMap = this->perlin.octave2D_11(worldX, worldZ, 2);

            BlockID surfaceBlock;
            if (y < realSeaLevel) {
                surfaceBlock = BLOCK_DIRT;
            } else if (temperature > 0.7) {
                surfaceBlock = BLOCK_STONE;
            } else {
                surfaceBlock = BLOCK_GRASS;
            }

            chunk->setBlock(surfaceBlock, {xx, y, zz});

            if (y > realSeaLevel && populationMap > 0.54) {
                std::pair<Vec3i, BlockID> treePrefab[] = {
                    std::make_pair(Vec3i(0, 0, 0), BLOCK_LOG),
                    std::make_pair(Vec3i(0, 1, 0), BLOCK_LOG),
                    std::make_pair(Vec3i(0, 2, 0), BLOCK_LOG),
                    std::make_pair(Vec3i(0, 3, 0), BLOCK_LOG),
                    std::make_pair(Vec3i(0, 4, 0), BLOCK_LOG),

                    std::make_pair(Vec3i(1, 4, 0), BLOCK_LEAVES),
                    std::make_pair(Vec3i(-1, 4, 0), BLOCK_LEAVES),
                    std::make_pair(Vec3i(0, 4, 1), BLOCK_LEAVES),
                    std::make_pair(Vec3i(0, 4, -1), BLOCK_LEAVES),
                    std::make_pair(Vec3i(1, 4, 1), BLOCK_LEAVES),
                    std::make_pair(Vec3i(-1, 4, -1), BLOCK_LEAVES),
                    std::make_pair(Vec3i(1, 4, -1), BLOCK_LEAVES),
                    std::make_pair(Vec3i(-1, 4, 1), BLOCK_LEAVES),

                    std::make_pair(Vec3i(0, 5, 0), BLOCK_LEAVES)
                };
                for (std::pair<Vec3i, BlockID> prefab : treePrefab) {
                    Vec3i offset = prefab.first;
                    int blockID = prefab.second;
                    Vec3i pos = {xx + offset.x, y + 1 + offset.y, zz + offset.z};

                    if (pos.x >= 0 && pos.y >= 0 && pos.z >= 0 &&
                        pos.x < CHUNK_SIZE_XZ && pos.y < CHUNK_SIZE_Y && pos.z < CHUNK_SIZE_XZ)
                    chunk->setBlock(blockID, pos);
                }
            }

            for (int depth = 1; depth < 4; ++depth) {
                int depthY = y - depth;
                if (depthY < 0) break; // Avoid out-of-bounds

                if (depth < 3) {
                    chunk->setBlock(BLOCK_DIRT, {xx, depthY, zz});
                } else {
                    chunk->setBlock(BLOCK_STONE, {xx, depthY, zz});
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