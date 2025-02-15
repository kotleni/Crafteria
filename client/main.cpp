#define GL_GLEXT_PROTOTYPES

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

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

class Chunk {
public:
    Chunk(Vec3i position): position(position) { }

    Vec3i position;

    std::vector<Block*> blocks;

    void setBlock(BlockID id, Vec3i pos) {
        for (Block *cube : blocks) {
            if (cube->position == pos) {
                cube->id = id; // Replace id
                return;
            }
        }

        blocks.push_back(new Block(pos));
    }

    Block* getBlock(Vec3i pos) {
        for (Block *cube : blocks) {
            if (cube->position == pos) {
                return cube;
            }
        }
        return nullptr;
    }
};

class World {
public:
    std::vector<Chunk*> chunks;

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
        for (Chunk *chunk : this->chunks) {
            chunk->setBlock(id, pos);
        }
    }
};

GLfloat cube_vertices[] = {
    // Positions          // Normals          // Texture Coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f
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

/**
 * Cached chunks renderer
 */
class ChunksRenderer {
    public:
        void renderChunks(std::vector<Chunk*> chunks, Shader *shader) {
            for (const auto &chunk : chunks) {
                for (const auto &block : chunk->blocks) {
                    glm::vec3 pos = {block->position.x, block->position.y, block->position.z};
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);

                    shader->use();
                    shader->setMat4("model", model);

                    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                }
            }
        }
};

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Chunk Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1400, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    glewInit();

    Image *cobblestoneImage = Image::load("cobblestone");
    {
        GLint textureID = 0;
        GLenum format;
        if (cobblestoneImage->nrComponents == 1)
            format = GL_RED;
        else if (cobblestoneImage->nrComponents == 3)
            format = GL_RGB;
        else if (cobblestoneImage->nrComponents == 4)
            format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, cobblestoneImage->width, cobblestoneImage->height, 0, format, GL_UNSIGNED_BYTE, cobblestoneImage->raw);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    stbi_image_free(cobblestoneImage->raw);
    delete cobblestoneImage;

    Shader *shader = Shader::load("cube");

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

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

        const Uint8* state = SDL_GetKeyboardState(NULL);
        float camera_speed = 0.1f;
        if (state[SDL_SCANCODE_W]) camera_pos += camera_speed * camera_front;
        if (state[SDL_SCANCODE_S]) camera_pos -= camera_speed * camera_front;
        if (state[SDL_SCANCODE_A]) camera_pos -= glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;
        if (state[SDL_SCANCODE_D]) camera_pos += glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;

        glm::mat4 view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        shader->setMat4("view", view);
        shader->setMat4("projection", projection);

        glBindVertexArray(vao);
        chunksRenderer->renderChunks(world->chunks, shader);
        SDL_GL_SwapWindow(window);
    }
    SDL_Quit();
    return 0;
}
