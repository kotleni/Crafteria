#include "Player.h"

glm::mat4 Player::getViewMatrix() {
    return this->view;
}

glm::vec3 Player::getPosition() {
    return this->position;
}

void Player::setPosition(glm::vec3 p) {
    this->position = p;
    this->updateViewMatrix();
}

void Player::moveRelative(glm::vec3 p) {
    this->position += p;
    this->updateViewMatrix();
}

void Player::updateViewMatrix() {
    this->view = glm::lookAt(this->position, this->position + camera_front, camera_up);
}