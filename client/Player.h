#ifndef PLAYER_H
#define PLAYER_H

#include <glm/glm.hpp>

class Player {
public:
    Player(): position({8, 20, 8}) { }

    glm::vec3 position;
};

#endif //PLAYER_H
