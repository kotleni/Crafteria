#define GL_GLEXT_PROTOTYPES

#include <array>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <thread>
#include <unordered_map>

#include "Shader.h"
#include "Image.h"
#include "PerlinNoise.h"
#include "Math/Vec3i.h"
#include "constants.h"
#include "Player.h"
#include "World.h"
#include "BlocksIds.h"

struct Clock
{
    uint32_t last_tick_time = 0;
    uint32_t delta = 0;

    void tick()
    {
        uint32_t tick_time = SDL_GetTicks();
        delta = tick_time - last_tick_time;
        last_tick_time = tick_time;
    }
};

Clock globalClock;

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
    ChunksRenderer() {

    }

    void renderChunks(std::vector<Chunk *> chunks, Shader *shader, Vec3i playerPos) {
        for (const auto &chunk: chunks) {
            double distance = (chunk->position * CHUNK_SIZE_XYZ).distanceTo(playerPos);
            if (distance > CHUNK_RENDERING_DISTANCE_IN_BLOCKS) {
                continue;
            }
            BakedChunk *bakedChunk = chunk->bakedChunk;

            // Chunk is not baked yet?
            if (bakedChunk == nullptr) continue;

            shader->use();
            glDisable(GL_BLEND);
            for (auto &part: bakedChunk->chunkParts) {
                if (!part.hasBuffered()) {
                    part.bufferMesh();
                }
                glBindVertexArray(part.vao);

                shader->setBool("isSolid", part.isSolid);

                glm::vec3 pos = {chunk->position.x, chunk->position.y, chunk->position.z};
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);

                shader->setMat4("model", model);

                glBindTexture(GL_TEXTURE_2D, part.blockID - 1);
                glDrawElements(GL_TRIANGLES, part.indices.size(), GL_UNSIGNED_INT, 0);
            }

            // Draw not-solid
            glEnable(GL_BLEND);
            for (auto &part: bakedChunk->liquidChunkParts) {
                if (!part.hasBuffered()) {
                    part.bufferMesh();
                }
                glBindVertexArray(part.vao);

                shader->setBool("isSolid", part.isSolid);

                glm::vec3 pos = {chunk->position.x, chunk->position.y, chunk->position.z};
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);

                shader->setMat4("model", model);

                glBindTexture(GL_TEXTURE_2D, part.blockID - 1);
                glDrawElements(GL_TRIANGLES, part.indices.size(), GL_UNSIGNED_INT, 0);
            }
        }
    }
};

void loadImageToGPU(std::string fileName, GLuint textureID) {
    Image *cobblestoneImage = Image::load(fileName);
    {
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
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Chunk Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1400, 900,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    glewInit();

    std::pair<std::string, BlockID> blocksData[10] = {
        std::pair("cobblestone", BLOCK_COBBLESTONE),
        std::pair("dirt", BLOCK_DIRT),
        std::pair("grass", BLOCK_GRASS),
        std::pair("lava", BLOCK_LAVA),
        std::pair("water", BLOCK_WATER),
        std::pair("oak_leaves", BLOCK_LEAVES),
        std::pair("oak_log", BLOCK_LOG),
        std::pair("oak_planks", BLOCK_PLANKS),
        std::pair("stone", BLOCK_STONE),
        std::pair("sand", BLOCK_SAND),
    };
    for (int i = 0; i < 10; i++) {
        loadImageToGPU(blocksData[i].first, blocksData[i].second - 1);
        std::cout << "Loaded block data " << blocksData[i].first << std::endl;
    }

    Shader *shader = Shader::load("cube");

    // TODO
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND);

    glEnable(GL_DEPTH_TEST);

    auto *chunksRenderer = new ChunksRenderer();
    auto *world = new World(SDL_GetTicks());
    int x = 0;
    int z = 0;
    // world->generateFilledChunk({0, 0, 0});

    glm::vec3 camera_front(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up(0.0f, 1.0f, 0.0f);

    bool isMouseRelative = false;
    // SDL_SetRelativeMouseMode(SDL_TRUE);

    bool running = true;
    auto lastTime = SDL_GetTicks();
    int frameCount = 0;
    int stableFrameCount = 0;
    while (running) {
        globalClock.tick();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            processMouseMotion(event, camera_front);

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_F3:
                        isMouseRelative = !isMouseRelative;
                        SDL_SetRelativeMouseMode(static_cast<SDL_bool>(isMouseRelative));
                        break;
                    // case SDLK_r:
                    //     x+=1;
                    // if (x >= 4) {
                    //     x = 0;
                    //     z+=1;
                    // }
                    //     world->generateFilledChunk({x, 0, z});
                    //     break;
                }
            }
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);
        float camera_speed = 0.01f * globalClock.delta;
        if (state[SDL_SCANCODE_W]) world->player->position += camera_speed * camera_front;
        if (state[SDL_SCANCODE_S]) world->player->position -= camera_speed * camera_front;
        if (state[SDL_SCANCODE_A]) world->player->position -= glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;
        if (state[SDL_SCANCODE_D]) world->player->position += glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;

        glm::mat4 view = glm::lookAt(world->player->position, world->player->position + camera_front, camera_up);
        glm::mat4 projection = glm::perspective(glm::radians(75.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        shader->setMat4("view", view);
        shader->setMat4("projection", projection);
        shader->setVec3("lightPos", glm::vec3(10.0f, 10.0f, 10.0f));
        shader->setVec3("viewPos", world->player->position);

        // glBindVertexArray(vao);
        Vec3i playerPos = {static_cast<int>(world->player->position.x), static_cast<int>(world->player->position.y), static_cast<int>(world->player->position.z)};
        chunksRenderer->renderChunks(world->chunks, shader, playerPos);
        SDL_GL_SwapWindow(window);

        frameCount++;
        if (SDL_GetTicks() - lastTime > 1000) {
            lastTime = SDL_GetTicks();

            stableFrameCount = frameCount;
            frameCount = 0;

            std::string title = "FPS: " + std::to_string(stableFrameCount);
            title += " | Chunks loaded: " + std::to_string(world->chunks.size());
            title += " | POS: " + std::to_string(world->player->position.x) + " " + std::to_string(world->player->position.y) + " " + std::to_string(world->player->position.z);
            SDL_SetWindowTitle(window, title.c_str());
        }
    }
    SDL_Quit();
    return 0;
}
