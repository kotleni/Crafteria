#ifndef RAY_H
#define RAY_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Ray {
public:
    Ray(const glm::vec3 &position, const glm::vec3 &direction);

    void step(float scale);

    const glm::vec3 &getEnd() const;

    float getLength() const;

private:
    glm::vec3 m_rayStart;
    glm::vec3 m_rayEnd;
    glm::vec3 m_direction;
};

#endif