#include "Block.h"

bool Block::isSolid() {
    return id != BLOCK_AIR && id != BLOCK_WATER && id != BLOCK_LAVA;
}