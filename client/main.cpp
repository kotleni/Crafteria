#define GL_GLEXT_PROTOTYPES

#include <array>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "Shader.h"
#include "Image.h"

#define CHUNK_SIZE_XYZ 16

class Vec3i {
public:
    Vec3i(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    bool operator==(const Vec3i &other) const {
        return this->x == other.x && this->y == other.y && this->z == other.z;
    }

    Vec3i operator+(const Vec3i &other) const {
        return {this->x + other.x, this->y + other.y, this->z + other.z};
    }

    int x;
    int y;
    int z;
};

typedef int BlockID;

class Block {
public:
    Vec3i position;
    BlockID id;
};

// TODO: Replace with real hash
int fakeHashIndex = 0;

class Chunk {
public:
    Chunk(Vec3i position): position(position) {
        this->hash = fakeHashIndex++;
    }

    int hash = -1;
    Vec3i position;

    std::vector<Block *> blocks;

    void setBlock(BlockID id, Vec3i pos) {
        for (Block *cube: blocks) {
            if (cube->position == pos) {
                cube->id = id; // Replace id
                return;
            }
        }

        blocks.push_back(new Block(pos));
    }

    Block *getBlock(Vec3i pos) {
        for (Block *cube: blocks) {
            if (cube->position == pos) {
                return cube;
            }
        }
        return nullptr;
    }
};

class World {
public:
    std::vector<Chunk *> chunks;

    void generateFilledChunk(Vec3i pos) {
        Chunk *chunk = new Chunk(pos);
        this->chunks.push_back(chunk);

        for (int x = 0; x < CHUNK_SIZE_XYZ; ++x) {
            for (int y = 0; y < CHUNK_SIZE_XYZ; ++y) {
                for (int z = 0; z < CHUNK_SIZE_XYZ; ++z) {
                    chunk->setBlock(1, {x, y, z});
                }
            }
        }
    }

    void setBlock(BlockID id, Vec3i pos) {
        for (Chunk *chunk: this->chunks) {
            chunk->setBlock(id, pos);
        }
    }
};

GLfloat cube_vertices[] = {
    // Positions          // Normals          // Texture Coords
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,

    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f
};


GLuint cube_indices[] = {
    0, 1, 2, 2, 3, 0,
    1, 5, 6, 6, 2, 1,
    5, 4, 7, 7, 6, 5,
    4, 0, 3, 3, 7, 4,
    3, 2, 6, 6, 7, 3,
    4, 5, 1, 1, 0, 4
};

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

void processMouseMotion(SDL_Event &event, glm::vec3 &camera_front) {
    if (event.type == SDL_MOUSEMOTION) {
        float xoffset = event.motion.xrel * 0.1f;
        float yoffset = -event.motion.yrel * 0.1f;

        yaw += xoffset;
        pitch += yoffset;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        camera_front = glm::normalize(front);
    }
}

struct BakedChunkPart {
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    BlockID blockID;
};

struct BakedChunk {
    std::vector<BakedChunkPart> chunkParts;
};

/**
 * Cached chunks renderer
 */
class ChunksRenderer {
private:
    std::pmr::unordered_map<int, BakedChunk *> cachedBakedChunks;

    BakedChunk *bakeChunk(Chunk *chunk) {
        if (cachedBakedChunks.contains(chunk->hash)) {
            return cachedBakedChunks.at(chunk->hash);
        }
        auto bakedChunk = new BakedChunk();

        for (const auto &block: chunk->blocks) {
            Block *currentBlock = block;
            Vec3i neighborOffsets[] = {
                Vec3i(0, 0, -1), // front
                Vec3i(0, 0, 1), // back
                Vec3i(0, -1, 0), // bottom
                Vec3i(0, 1, 0), // top
                Vec3i(-1, 0, 0), // left
                Vec3i(1, 0, 0), // right
            };

            glm::vec3 faceDirections[] = {
                glm::vec3(0, 0, -1), // front
                glm::vec3(0, 0, 1), // back
                glm::vec3(0, -1, 0), // bottom
                glm::vec3(0, 1, 0), // top
                glm::vec3(-1, 0, 0), // left
                glm::vec3(1, 0, 0), // right
            };
            // Check each block's neighbors to determine which faces should be visible
            for (int i = 0; i < 6; ++i) {
                // 6 faces per block
                glm::vec3 faceDirection;
                std::vector<GLfloat> vertices;
                std::vector<GLuint> indices;

                // Determine the direction for each face
                faceDirection = faceDirections[i];

                // Check if the neighboring block exists or is air (to render the face)
                //glm::vec3 neighborPos = glm::vec3(currentBlock->position.x, currentBlock->position.y, currentBlock->position.z) + faceDirection;
                Vec3i neighborPos = currentBlock->position + neighborOffsets[i];

                if (chunk->getBlock(Vec3i(neighborPos.x, neighborPos.y, neighborPos.z)) == nullptr) {
                    // Generate vertices and indices for the visible face
                    int vertexOffset = vertices.size() / 8;

                    // HOLLY CRAP????
                    if (faceDirection == glm::vec3(0, 0, -1)) {
                        // front
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(1.0f);
                    } else if (faceDirection == glm::vec3(0, 0, 1)) {
                        // back
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(1.0f);
                    } else if (faceDirection == glm::vec3(0, -1, 0)) {
                        // bottom
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(1.0f);
                    } else if (faceDirection == glm::vec3(0, 1, 0)) {
                        // top
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(1.0f);
                    } else if (faceDirection == glm::vec3(-1, 0, 0)) {
                        // left
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(currentBlock->position.x - 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(1.0f);
                    } else if (faceDirection == glm::vec3(1, 0, 0)) {
                        // right
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y - 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(0.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z + 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(1.0f);
                        vertices.push_back(1.0f);
                        vertices.push_back(currentBlock->position.x + 0.5f);
                        vertices.push_back(currentBlock->position.y + 0.5f);
                        vertices.push_back(currentBlock->position.z - 0.5f);
                        vertices.push_back(faceDirection.x);
                        vertices.push_back(faceDirection.y);
                        vertices.push_back(faceDirection.z);
                        vertices.push_back(0.0f);
                        vertices.push_back(1.0f);
                    }

                    indices.push_back(vertexOffset + 0);
                    indices.push_back(vertexOffset + 1);
                    indices.push_back(vertexOffset + 2);
                    indices.push_back(vertexOffset + 2);
                    indices.push_back(vertexOffset + 3);
                    indices.push_back(vertexOffset + 0);
                }

                // If there are vertices and indices for this face, create a part and add it to the baked chunk
                if (!vertices.empty() && !indices.empty()) {
                    BakedChunkPart part;
                    part.vertices = std::move(vertices);
                    part.indices = std::move(indices);
                    part.blockID = currentBlock->id;
                    bakedChunk->chunkParts.push_back(part);
                }
            }
        }

        std::cout << "Baked new chunk " << chunk->hash << std::endl;
        cachedBakedChunks.insert(std::make_pair(chunk->hash, bakedChunk));
        return bakedChunk;
    }

public:
    void renderChunks(std::vector<Chunk *> chunks, Shader *shader) {
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        for (const auto &chunk: chunks) {
            BakedChunk *bakedChunk = bakeChunk(chunk);

            glBindVertexArray(vao);
            for (const auto &part: bakedChunk->chunkParts) {
                glBindVertexArray(vao);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, part.vertices.size() * sizeof(GLfloat), part.vertices.data(),
                             GL_STATIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, part.indices.size() * sizeof(GLuint), part.indices.data(),
                             GL_STATIC_DRAW);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) 0);
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
                glEnableVertexAttribArray(1);

                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));
                glEnableVertexAttribArray(2);

                glm::vec3 pos = {chunk->position.x, chunk->position.y, chunk->position.z};
                pos *= CHUNK_SIZE_XYZ; // Scale chunk pos
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);

                shader->use();
                shader->setMat4("model", model);

                glDrawElements(GL_TRIANGLES, part.indices.size(), GL_UNSIGNED_INT, 0);
            }
        }
    }
};


int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Chunk Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1400, 900,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    glewInit();

    Image *cobblestoneImage = Image::load("cobblestone"); {
        GLint textureID = 0;
        GLenum format;
        if (cobblestoneImage->nrComponents == 1)
            format = GL_RED;
        else if (cobblestoneImage->nrComponents == 3)
            format = GL_RGB;
        else if (cobblestoneImage->nrComponents == 4)
            format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, cobblestoneImage->width, cobblestoneImage->height, 0, format,
                     GL_UNSIGNED_BYTE, cobblestoneImage->raw);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    stbi_image_free(cobblestoneImage->raw);
    delete cobblestoneImage;

    Shader *shader = Shader::load("cube");

    // TODO
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);

    auto *chunksRenderer = new ChunksRenderer();
    auto *world = new World();
    world->generateFilledChunk({0, 0, 0});

    glm::vec3 camera_pos(8.0f, 8.0f, 20.0f);
    glm::vec3 camera_front(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up(0.0f, 1.0f, 0.0f);

    SDL_SetRelativeMouseMode(SDL_TRUE);

    bool running = true;
    while (running) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            processMouseMotion(event, camera_front);
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        float camera_speed = 0.1f;
        if (state[SDL_SCANCODE_W]) camera_pos += camera_speed * camera_front;
        if (state[SDL_SCANCODE_S]) camera_pos -= camera_speed * camera_front;
        if (state[SDL_SCANCODE_A]) camera_pos -= glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;
        if (state[SDL_SCANCODE_D]) camera_pos += glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;

        glm::mat4 view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
        glm::mat4 projection = glm::perspective(glm::radians(75.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        shader->setMat4("view", view);
        shader->setMat4("projection", projection);
        shader->setVec3("lightPos", glm::vec3(10.0f, 10.0f, 10.0f));
        shader->setVec3("viewPos", camera_pos);

        // glBindVertexArray(vao);
        chunksRenderer->renderChunks(world->chunks, shader);
        SDL_GL_SwapWindow(window);
    }
    SDL_Quit();
    return 0;
}
