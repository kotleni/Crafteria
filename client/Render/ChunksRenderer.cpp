#include "ChunksRenderer.h"

std::array<Plane, 6> ChunksRenderer::extractFrustumPlanes(const glm::mat4 &matrix) {
    std::array<Plane, 6> planes;

    for (int i = 0; i < 6; i++) {
        int sign = (i % 2 == 0) ? 1 : -1;
        planes[i].normal = glm::vec3(
            matrix[0][3] + sign * matrix[0][i / 2],
            matrix[1][3] + sign * matrix[1][i / 2],
            matrix[2][3] + sign * matrix[2][i / 2]
        );
        planes[i].distance = matrix[3][3] + sign * matrix[3][i / 2];

        // Normalize the plane
        float length = glm::length(planes[i].normal);
        planes[i].normal /= length;
        planes[i].distance /= length;
    }

    return planes;
}

bool ChunksRenderer::isChunkInFrustum(const std::array<Plane, 6> &frustumPlanes, const glm::vec3 &chunkMin, const glm::vec3 &chunkMax) {
    for (const auto &plane : frustumPlanes) {
        // Check if all 8 corners of the chunk are outside the plane
        int outCount = 0;
        for (int x = 0; x < 2; x++) {
            for (int y = 0; y < 2; y++) {
                for (int z = 0; z < 2; z++) {
                    glm::vec3 corner = glm::vec3(
                        x ? chunkMax.x : chunkMin.x,
                        y ? chunkMax.y : chunkMin.y,
                        z ? chunkMax.z : chunkMin.z
                    );
                    if (!plane.isInFront(corner)) {
                        outCount++;
                    }
                }
            }
        }
        // If all corners are outside one plane, the chunk is fully outside the frustum
        if (outCount == 8) return false;
    }
    return true;
}

#define CUBE_MINUS_V -0.01f
#define CUBE_PLUS_V 1.01f

static const GLfloat blockVertices[] = {
    CUBE_MINUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V,CUBE_MINUS_V, CUBE_PLUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V, CUBE_PLUS_V,
    CUBE_MINUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V, CUBE_PLUS_V,
    CUBE_MINUS_V,CUBE_MINUS_V, CUBE_PLUS_V,
    CUBE_MINUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_MINUS_V,CUBE_MINUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V,CUBE_MINUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V,CUBE_MINUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_MINUS_V, CUBE_PLUS_V, CUBE_PLUS_V,
    CUBE_PLUS_V,CUBE_MINUS_V, CUBE_PLUS_V
};

ChunksRenderer::ChunksRenderer(std::unordered_map<BlockID, GLuint> glTextures) {
    this->glTextures = glTextures;

    glGenVertexArrays(1, &vaoSelection);
    glGenBuffers(1, &vboSelection);
    glGenBuffers(1, &eboSelection);

    glBindVertexArray(vaoSelection);
    glBindBuffer(GL_ARRAY_BUFFER, vboSelection);
    glBufferData(GL_ARRAY_BUFFER, sizeof(blockVertices), blockVertices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,                                // attribute. No particular reason for 1, but must match the layout in the shader.
        3,                                // size
        GL_FLOAT,                         // type
        GL_FALSE,                         // normalized?
        0,                                // stride
        (void*)0                          // array buffer offset
    );
    glEnableVertexAttribArray(0);
}

void ChunksRenderer::renderChunks(World *world, Shader *shader, Shader *waterShader, Shader *selectionShader, Vec3i playerPos) {
    lastCountOfTotalVertices = 0;

    // Copy chunks array
    std::vector<Chunk *> chunks = world->chunks;

    glm::mat4 viewProjection = projection * world->player->getViewMatrix();
    glm::vec3 pos;

    auto frustumPlanes = extractFrustumPlanes(viewProjection);

    shader->use();
    shader->setMat4("view", world->player->getViewMatrix());
    shader->setMat4("projection", projection);
    shader->setVec3("lightPos", this->lightPos);
    shader->setVec3("viewPos", world->player->getPosition());

    glDisable(GL_BLEND);

    if (isUseSingleTexture)
        glBindTexture(GL_TEXTURE_2D, 1);

    // Draw all solid & unload if needed
    for (const auto &chunk: chunks) {
        if (chunk->isNeedToUnload) {
            world->unloadChunk(chunk);
            continue;
        }

        Vec3i playerChunkPos = {
            playerPos.x / CHUNK_SIZE_XZ, 0,
            playerPos.z / CHUNK_SIZE_XZ
        };
        double distance = (chunk->position).distanceTo(playerChunkPos);
        if (distance > CHUNK_RENDERING_DISTANCE_IN_BLOCKS) {
            continue;
        }
        BakedChunk *bakedChunk = chunk->getBakedChunk();

        // Chunk is not baked yet?
        if (bakedChunk == nullptr) continue;

        pos.x = chunk->position.x * CHUNK_SIZE_XZ;
        pos.y = chunk->position.y * CHUNK_SIZE_Y;
        pos.z = chunk->position.z * CHUNK_SIZE_XZ;

        glm::vec3 chunkMin = pos;
        glm::vec3 chunkMax = pos + glm::vec3(CHUNK_SIZE_XZ, CHUNK_SIZE_Y, CHUNK_SIZE_XZ);

        // Frustum culling check
        if (!isChunkInFrustum(frustumPlanes, chunkMin, chunkMax)) {
            continue;
        }

        for (auto &part: bakedChunk->chunkParts) {
            if (!part.hasBuffered()) {
                part.bufferMesh();
            }
            glBindVertexArray(part.vao);

            shader->setVec3("pos", pos);

            if (!isUseSingleTexture)
                glBindTexture(GL_TEXTURE_2D, this->glTextures[part.blockID]);
            glDrawElements(GL_TRIANGLES, part.indices.size(), GL_UNSIGNED_INT, nullptr);
            lastCountOfTotalVertices += part.vertices.size() / 9; // Verticles count
        }
    }

    // Copy actual chunks array
    chunks = world->chunks;

    waterShader->use();
    waterShader->setMat4("view", world->player->getViewMatrix());
    waterShader->setMat4("projection", projection);
    waterShader->setVec3("lightPos", this->lightPos);
    waterShader->setVec3("viewPos", world->player->getPosition());
    waterShader->setFloat("time", SDL_GetTicks() / 1000.0f);

    glEnable(GL_BLEND);

    // Draw all liquid
    for (const auto &chunk: chunks) {
        double distance = (chunk->position * CHUNK_SIZE_XZ).distanceTo({playerPos.x, 0, playerPos.z});
        if (distance > CHUNK_RENDERING_DISTANCE_IN_BLOCKS) {
            continue;
        }
        BakedChunk *bakedChunk = chunk->getBakedChunk();

        // Chunk is not baked yet?
        if (bakedChunk == nullptr) continue;;

        pos.x = chunk->position.x * CHUNK_SIZE_XZ;
        pos.y = chunk->position.y * CHUNK_SIZE_Y;
        pos.z = chunk->position.z * CHUNK_SIZE_XZ;

        // Draw not-solid
        for (auto &part: bakedChunk->liquidChunkParts) {
            if (!part.hasBuffered()) {
                part.bufferMesh();
            }
            glBindVertexArray(part.vao);

            waterShader->setVec3("pos", pos);
            waterShader->setVec3("worldPos", pos);

            if (!isUseSingleTexture)
                glBindTexture(GL_TEXTURE_2D, this->glTextures[part.blockID]);
            glDrawElements(GL_TRIANGLES, part.indices.size(), GL_UNSIGNED_INT, nullptr);
            lastCountOfTotalVertices += part.vertices.size() / 9; // Verticles count
        }
    }

    // Render selected block
    {
        glEnable(GL_BLEND);

        selectionShader->use();
        selectionShader->setMat4("view", world->player->getViewMatrix());
        selectionShader->setMat4("projection", projection);
        selectionShader->setVec3("pos", glm::vec3(targetBlock.x, targetBlock.y, targetBlock.z));

        glBindVertexArray(vaoSelection);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(blockVertices));
    }
}
