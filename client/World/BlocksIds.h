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
    BLOCK_SAND = 10
};

struct BlockData {
    std::string name;
    BlockID blockID;
    float atlasX, atlasY;
    bool isSolid;
};

inline const BlockData BLOCKS_DATA[10] = {
    { "cobblestone", BLOCK_COBBLESTONE, 0, 32, true },
    { "dirt", BLOCK_DIRT, 0, 32, true },
    { "grass", BLOCK_GRASS, 32, 0, true },
    { "lava", BLOCK_LAVA, 0, 64, false },
    { "water", BLOCK_WATER, 64, 64, false },
    { "oak_leaves", BLOCK_LEAVES, 96, 0, true },
    { "oak_log", BLOCK_LOG, 64, 32, true },
    { "oak_planks", BLOCK_PLANKS, 96, 32, true },
    { "stone", BLOCK_STONE, 32, 32, true },
    { "sand", BLOCK_SAND, 32, 64, true },
};

#endif