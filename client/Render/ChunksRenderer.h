#ifndef CHUNKSRENDERER_H
#define CHUNKSRENDERER_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

#include "../World/World.h"
#include "../Shader.h"

struct Plane {
    glm::vec3 normal;
    float distance;

    // Returns true if a point is in front of the plane
    [[nodiscard]] bool isInFront(const glm::vec3 &point) const {
        return glm::dot(normal, point) + distance >= 0;
    }
};

/**
 * Cached chunks renderer
 */
class ChunksRenderer {
    glm::mat4 projection = glm::perspective(glm::radians(75.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
    glm::vec3 lightPos = glm::vec3(10.0f, 10.0f, 10.0f);
    glm::mat4 mat4One = glm::mat4(1.0f);

    std::unordered_map<BlockID, GLuint> glTextures;

    // Extracts the frustum planes from the view-projection matrix
    std::array<Plane, 6> extractFrustumPlanes(const glm::mat4 &matrix);
    bool isChunkInFrustum(const std::array<Plane, 6> &frustumPlanes, const glm::vec3 &chunkMin, const glm::vec3 &chunkMax);
public:
    bool isUseSingleTexture = false;
    int lastCountOfTotalVertices = 0;

    ChunksRenderer(std::unordered_map<BlockID, GLuint> glTextures): glTextures(glTextures) {}

    void renderChunks(World* world, Shader *shader, Shader *waterShader, Vec3i playerPos);
};

#endif //CHUNKSRENDERER_H
