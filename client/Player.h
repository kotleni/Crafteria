#ifndef PLAYER_H
#define PLAYER_H

#include <glm/glm.hpp>

#include "constants.h"

class Player {
public:
    Player(): position({8, CHUNK_SIZE_Y - 30 , 8}) { }

    glm::vec3 position;
};

#endif //PLAYER_H
