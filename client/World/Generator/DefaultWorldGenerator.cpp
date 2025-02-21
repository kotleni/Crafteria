#include "DefaultWorldGenerator.h"

struct BiomeConfig {
    unsigned short id;
    std::string name;

    BlockID topBlockId;
    BlockID mediumBlockId;

    float baseLevel;
    float heightMultiplier;

    bool isWatery;
};


// Ordering is matter, bcs chunks changes smoothly depends by order
std::vector<BiomeConfig> biomeConfigs = {
    { 0, "forest", BLOCK_GRASS, BLOCK_DIRT, 64, 8.0f, true },
    { 2, "plain", BLOCK_GRASS, BLOCK_DIRT, 70, 4.0f, true },
    { 3, "desert", BLOCK_SAND, BLOCK_SAND, 64, 7.0f, false },
    { 5, "ocean", BLOCK_SAND, BLOCK_SAND, 44, 16.0f, true },
    { 4, "mountains", BLOCK_STONE, BLOCK_STONE, 64, 16.0f, false },
};

void DefaultWorldGenerator::generateChunk(Chunk *chunk) {
    constexpr float scale = 0.08f;
    constexpr int octaves = 6;
    constexpr int realSeaLevel = 67;
    constexpr float biomeBlendScale = 0.009f;

    int chunkWorldX = chunk->position.x * CHUNK_SIZE_XZ;
    int chunkWorldZ = chunk->position.z * CHUNK_SIZE_XZ;

    for (int xx = 0; xx < CHUNK_SIZE_XZ; ++xx) {
        for (int zz = 0; zz < CHUNK_SIZE_XZ; ++zz) {
            int worldX = chunkWorldX + xx;
            int worldZ = chunkWorldZ + zz;

            double biomeNoise = this->perlin.octave2D_01(worldX * biomeBlendScale, worldZ * biomeBlendScale, octaves);
            double biomeLerpFactor = biomeNoise * (biomeConfigs.size() - 1);
            int biomeIndex1 = static_cast<int>(biomeLerpFactor);
            int biomeIndex2 = std::min(biomeIndex1 + 1, static_cast<int>(biomeConfigs.size() - 1));
            float blendFactor = biomeLerpFactor - biomeIndex1;

            BiomeConfig &biome1 = biomeConfigs[biomeIndex1];
            BiomeConfig &biome2 = biomeConfigs[biomeIndex2];
            BiomeConfig &activeBiome = blendFactor > 0.5 ? biome2 : biome1;

            float heightMultiplier = biome1.heightMultiplier * (1.0 - blendFactor) + biome2.heightMultiplier * blendFactor;
            float baseLevel = biome1.baseLevel * (1.0 - blendFactor) + biome2.baseLevel * blendFactor;

            double yMod = this->perlin.octave2D_01(worldX * scale, worldZ * scale, octaves);
            int y = static_cast<int>(yMod * heightMultiplier) + baseLevel;

            assert(y >= 0 && y < CHUNK_SIZE_Y);

            BlockID surfaceBlock = (y >= realSeaLevel) ? activeBiome.topBlockId : activeBiome.mediumBlockId;
            BlockID mediumBlock = activeBiome.mediumBlockId;

            chunk->setBlock(surfaceBlock, {xx, y, zz});

            for (int depth = y - 1; depth >= 0; --depth) {
                chunk->setBlock(y - (depth - 1) < 3 ? mediumBlock : BLOCK_STONE, {xx, depth, zz});
            }

            if (y < realSeaLevel) {
                for (int waterY = y; waterY <= realSeaLevel; ++waterY) {
                    Vec3i pos = {xx, waterY, zz};
                    if (chunk->getBlock(pos) == nullptr)
                        chunk->setBlock(BLOCK_WATER, pos);
                }
            } else if (activeBiome.id == 0) { // FIXME: HAX
                double populationMap = this->perlin.octave2D_11(worldX, worldZ, 2);
                if (populationMap > 0.54) {
                    std::pair<Vec3i, BlockID> treePrefab[] = {
                        { {0, 0, 0}, BLOCK_LOG }, { {0, 1, 0}, BLOCK_LOG }, { {0, 2, 0}, BLOCK_LOG },
                        { {0, 3, 0}, BLOCK_LOG }, { {0, 4, 0}, BLOCK_LOG }, { {1, 4, 0}, BLOCK_LEAVES },
                        { {-1, 4, 0}, BLOCK_LEAVES }, { {0, 4, 1}, BLOCK_LEAVES }, { {0, 4, -1}, BLOCK_LEAVES },
                        { {1, 4, 1}, BLOCK_LEAVES }, { {-1, 4, -1}, BLOCK_LEAVES }, { {1, 4, -1}, BLOCK_LEAVES },
                        { {-1, 4, 1}, BLOCK_LEAVES }, { {0, 5, 0}, BLOCK_LEAVES }
                    };
                    for (auto &prefab : treePrefab) {
                        Vec3i offset = prefab.first;
                        Vec3i pos = {xx + offset.x, y + 1 + offset.y, zz + offset.z};
                        if (pos.x >= 0 && pos.y >= 0 && pos.z >= 0 && pos.x < CHUNK_SIZE_XZ && pos.y < CHUNK_SIZE_Y && pos.z < CHUNK_SIZE_XZ)
                            chunk->setBlock(prefab.second, pos);
                    }
                }
            }
        }
    }
}