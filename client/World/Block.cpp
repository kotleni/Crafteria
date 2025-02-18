#include "Block.h"

bool Block::isSolid() const {
    return id != BLOCK_AIR && id != BLOCK_WATER && id != BLOCK_LAVA;
}
