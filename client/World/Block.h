#ifndef BLOCK_H
#define BLOCK_H

#include "BlocksIds.h"
#include "../Math/Vec3i.h"

class Block {
public:
    Vec3i position;
    BlockID id;

    bool isSolid();
};

#endif //BLOCK_H
