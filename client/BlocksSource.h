#ifndef BLOCKS_SOURCE_H
#define BLOCKS_SOURCE_H

#include "Block.h"

/**
 * Abstract source of blocks
 */
class BlocksSource {
public:
    virtual Block *getBlock(Vec3i pos) = 0;
    virtual void setBlock(BlockID id, Vec3i pos) = 0;
};

#endif