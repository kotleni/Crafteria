#include "Block.h"

bool Block::isSolid() const {
    return id != BLOCK_AIR && id != BLOCK_WATER && id != BLOCK_LAVA;
}

BlockID Block::getId() const {
    return id;
}

Vec3i Block::getChunkPosition() const {
    return position;
}

void Block::setBlockId(BlockID id) {
    this->id = id;
}