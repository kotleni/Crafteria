#ifndef PLAYER_H
#define PLAYER_H

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "constants.h"

class Player {
private:
    glm::vec3 position;
    glm::mat4 view{};
public:
    glm::vec3 camera_front{};
    glm::vec3 camera_up{};

    Player(): position({8, CHUNK_SIZE_Y - 30 , 8}) {
        this->camera_front = glm::vec3(0, 0, -1);
        this->camera_up = glm::vec3(0, 1, 0);
        this->view = glm::lookAt(position, position + camera_front, camera_up);
    }

    glm::mat4 getViewMatrix();
    glm::vec3 getPosition();

    void updateViewMatrix();

    void setPosition(glm::vec3 p);
    void moveRelative(glm::vec3 p);
};

#endif //PLAYER_H
