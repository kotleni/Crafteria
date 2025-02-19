#ifndef BLOCK_H
#define BLOCK_H

#include "BlocksIds.h"
#include "../constants.h"
#include "../Math/Vec3i.h"

class Block {
    Vec3i position;
    BlockID id;
public:
    Block(Vec3i position, BlockID id) : position(position), id(id) {}

    BlockID getId() const;
    Vec3i getChunkPosition() const;
    void setBlockId(BlockID id);
    bool isSolid() const;
};

#endif //BLOCK_H
