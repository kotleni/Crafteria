#include "Block.h"

bool Block::isSolid() const {
    return id != BLOCK_AIR && id != BLOCK_WATER && id != BLOCK_LAVA;
}

bool Block::isFlora() const {
    return id == BLOCK_GRASS_BUSH || id == BLOCK_FLOWER_RED || id == BLOCK_TORCH;
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