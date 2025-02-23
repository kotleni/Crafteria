#ifndef BLOCKSIDS_H
#define BLOCKSIDS_H

#include <string>

typedef int BlockID;

enum BlocksIds: BlockID {
    BLOCK_AIR = 0,
    BLOCK_STONE = 1,
    BLOCK_COBBLESTONE = 2,
    BLOCK_DIRT = 3,
    BLOCK_GRASS = 4,
    BLOCK_PLANKS = 5,
    BLOCK_LOG = 6,
    BLOCK_LEAVES = 7,
    BLOCK_LAVA = 8,
    BLOCK_WATER = 9,
    BLOCK_SAND = 10,
    BLOCK_IRON_ORE = 11,
    BLOCK_GRASS_BUSH = 12,
    BLOCK_FLOWER_RED = 13,
    BLOCK_TORCH = 14,
    BLOCK_SNOW = 15,
};

struct BlockData {
    std::string name;
    BlockID blockID;
    float atlasX, atlasY;
    bool isSolid;
    bool isFlora;
};

inline const BlockData BLOCKS_DATA[15] = {
    { "cobblestone", BLOCK_COBBLESTONE, 0, 32, true, false },
    { "dirt", BLOCK_DIRT, 0, 32, true, false },
    { "grass", BLOCK_GRASS, 32, 0, true, false },
    { "lava", BLOCK_LAVA, 0, 64, false, false },
    { "water", BLOCK_WATER, 64, 64, false, false },
    { "oak_leaves", BLOCK_LEAVES, 96, 0, true, false },
    { "oak_log", BLOCK_LOG, 64, 32, true, false },
    { "oak_planks", BLOCK_PLANKS, 96, 32, true, false },
    { "stone", BLOCK_STONE, 32, 32, true, false },
    { "sand", BLOCK_SAND, 32, 64, true, false },
    { "iron_ore", BLOCK_IRON_ORE, 0, 0, true, false },
    { "grass_bush", BLOCK_GRASS_BUSH, 0, 0, false, true },
    { "flower_red", BLOCK_FLOWER_RED, 0, 0, false, true },
    { "torch", BLOCK_TORCH, 0, 0, false, true },
    { "snow", BLOCK_SNOW, 0, 0, true, false },
};

#endif