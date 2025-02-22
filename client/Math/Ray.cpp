#include "Ray.h"

Ray::Ray(const glm::vec3 &position, const glm::vec3 &direction)
    : m_rayStart(position)
    , m_rayEnd(position)
    , m_direction(direction)
{
}

void Ray::step(float scale)
{
    auto &p = m_rayEnd;

    m_rayEnd = p + (m_direction * scale);
}

const glm::vec3 &Ray::getEnd() const
{
    return m_rayEnd;
}

float Ray::getLength() const
{
    return glm::distance(m_rayStart, m_rayEnd);
}
