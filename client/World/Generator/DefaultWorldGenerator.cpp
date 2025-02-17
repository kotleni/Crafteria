#include "DefaultWorldGenerator.h"

void DefaultWorldGenerator::generateChunk(Chunk *chunk) {
    constexpr float scale = 0.08f;
    constexpr float heightMultiplier = 8.0f;
    constexpr int octaves = 5;
    constexpr int seaLevel = 12;
    constexpr int realSeaLevel = 15;

    int baseX = chunk->position.x * CHUNK_SIZE_XYZ;
    int baseZ = chunk->position.z * CHUNK_SIZE_XYZ;

    for (int x = 0; x < CHUNK_SIZE_XYZ; ++x) {
        for (int z = 0; z < CHUNK_SIZE_XYZ; ++z) {
            int xx = baseX + x;
            int zz = baseZ + z;
            double yMod = this->perlin.octave2D_01(xx * scale, zz * scale, octaves);
            int y = static_cast<int>(yMod * heightMultiplier) + seaLevel;

            double temperature = this->perlin.octave2D_11(xx * scale * 0.5, zz * scale * 0.5, octaves);

            BlockID surfaceBlock;
            if (y < realSeaLevel) {
                surfaceBlock = BLOCK_DIRT;
            } else if (temperature > 0.7) {
                surfaceBlock = BLOCK_STONE;
            } else {
                surfaceBlock = BLOCK_GRASS;
            }

            chunk->setBlock(surfaceBlock, {xx, y, zz});

            for (int depth = 1; depth < 16; ++depth) {
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