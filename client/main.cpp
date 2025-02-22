#define GL_GLEXT_PROTOTYPES

#include <array>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <execution>

#include "Shader.h"
#include "Image.h"
#include "PerlinNoise.h"
#include "Math/Vec3i.h"
#include "constants.h"
#include "Player.h"
#include "World/World.h"
#include "World/BlocksIds.h"

#include "GUI/imgui.h"
#include "GUI/imgui_impl_sdl2.h"
#include "GUI/imgui_impl_opengl3.h"

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

void processMouseMotion(SDL_Event &event, Player &player) {
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
        player.camera_front = glm::normalize(front);
        player.updateViewMatrix();
    }
}

struct Plane {
    glm::vec3 normal;
    float distance;

    // Returns true if a point is in front of the plane
    bool isInFront(const glm::vec3 &point) const {
        return glm::dot(normal, point) + distance >= 0;
    }
};

// Extracts the frustum planes from the view-projection matrix
std::array<Plane, 6> extractFrustumPlanes(const glm::mat4 &matrix) {
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

bool isChunkInFrustum(const std::array<Plane, 6> &frustumPlanes, const glm::vec3 &chunkMin, const glm::vec3 &chunkMax) {
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

/**
 * Cached chunks renderer
 */
class ChunksRenderer {
    glm::mat4 projection = glm::perspective(glm::radians(75.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
    glm::vec3 lightPos = glm::vec3(10.0f, 10.0f, 10.0f);
    glm::mat4 mat4One = glm::mat4(1.0f);

    std::unordered_map<BlockID, GLuint> glTextures;
public:
    bool isUseSingleTexture = false;
    int lastCountOfTotalVerticles = 0;

    ChunksRenderer(std::unordered_map<BlockID, GLuint> glTextures): glTextures(glTextures) { }

    void renderChunks(World* world, Shader *shader, Shader *waterShader, Vec3i playerPos) {
        lastCountOfTotalVerticles = 0;

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
                if (part.indices.size() > 3000 || part.indices.capacity() > 3000) {
                    std::cout << "Chunk part indices count/capacity limit. Bug?" << std::endl;
                    return;
                }

                if (!part.hasBuffered()) {
                    part.bufferMesh();
                }
                glBindVertexArray(part.vao);

                shader->setVec3("pos", pos);

                if (!isUseSingleTexture)
                    glBindTexture(GL_TEXTURE_2D, this->glTextures[part.blockID]);
                glDrawElements(GL_TRIANGLES, part.indices.size(), GL_UNSIGNED_INT, nullptr);
                lastCountOfTotalVerticles += part.vertices.size() / 9; // Verticles count
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
                lastCountOfTotalVerticles += part.vertices.size() / 9; // Verticles count
            }
        }
    }
};

GLuint loadImageToGPU(std::string fileName) {
    Image *image = Image::load(fileName);

    GLenum format;
    if (image->nrComponents == 1)
        format = GL_RED;
    else if (image->nrComponents == 3)
        format = GL_RGB;
    else if (image->nrComponents == 4)
        format = GL_RGBA;

    GLuint textureName;
    glGenTextures(1, &textureName);

    glBindTexture(GL_TEXTURE_2D, textureName);
    glTexImage2D(GL_TEXTURE_2D, 0, format, image->width, image->height, 0, format,
                 GL_UNSIGNED_BYTE, image->raw);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image->raw);
    delete image;

    return textureName;
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
    // Skip VIDEO memory usage for buffer warning
    if (severity == 0x826b) return;

    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
             ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
              type, severity, message );

    if (severity == 0x9146) {
        std::cout << "HANG" << std::endl;
        for (;;);
    }
}

bool raymarch(const glm::vec3& origin, const glm::vec3& direction, float maxLength,
              std::function<bool(const glm::ivec3&)> findBlock, glm::ivec3& hitBlock, glm::ivec3& hitNormal) {
    glm::vec3 rayDir = glm::normalize(direction);
    glm::vec3 invDir = 1.0f / rayDir;

    auto intbound = [](float s, float ds) -> float {
        if (ds > 0.0f) return (std::ceil(s) - s) / ds;
        if (ds < 0.0f) return (s - std::floor(s)) / -ds;
        return std::numeric_limits<float>::infinity();
    };

    glm::ivec3 pos = glm::floor(origin);
    glm::ivec3 step = glm::ivec3(glm::sign(rayDir));
    glm::vec3 tMax = glm::vec3(intbound(origin.x, rayDir.x),
                                intbound(origin.y, rayDir.y),
                                intbound(origin.z, rayDir.z));
    glm::vec3 tDelta = glm::vec3(glm::abs(invDir));
    float radius = maxLength / glm::length(rayDir);

    while (true) {
        if (findBlock(pos)) {
            hitBlock = pos;
            return true;
        }
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                if (tMax.x > radius) break;
                pos.x += step.x;
                tMax.x += tDelta.x;
                hitNormal = glm::ivec3(-step.x, 0, 0);
            } else {
                if (tMax.z > radius) break;
                pos.z += step.z;
                tMax.z += tDelta.z;
                hitNormal = glm::ivec3(0, 0, -step.z);
            }
        } else {
            if (tMax.y < tMax.z) {
                if (tMax.y > radius) break;
                pos.y += step.y;
                tMax.y += tDelta.y;
                hitNormal = glm::ivec3(0, -step.y, 0);
            } else {
                if (tMax.z > radius) break;
                pos.z += step.z;
                tMax.z += tDelta.z;
                hitNormal = glm::ivec3(0, 0, -step.z);
            }
        }
    }
    return false;
}

float crosshairVertices[] = {
    // Horizontal line
    -0.02f,  0.0f, 0.0f,  // Left point
     0.02f,  0.0f, 0.0f,  // Right point

    // Vertical line
     0.0f, -0.02f, 0.0f,  // Bottom point
     0.0f,  0.02f, 0.0f   // Top point
};

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Chunk Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1400, 900,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    bool isShowDebugMenu = true;

    // Enable debug
    // TODO: Disable for macOS (force)
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    // Loading images and store texture names
    // FIXME(hax): I think this is bad way
    std::unordered_map<BlockID, GLuint> glTextures;
    for (const BlockData& data: BLOCKS_DATA) {
        GLuint textureName = loadImageToGPU(data.name);
        glTextures[data.blockID] = textureName;

        std::cout << "Texture " << data.name << " loaded as " << textureName << ". (Block ID: " << data.blockID << ")" << std::endl;
    }

    Shader *shader = Shader::load("cube");
    Shader *waterShader = Shader::load("water");
    Shader *crosshairShader = Shader::load("crosshair");

    // TODO
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND);

    glEnable(GL_DEPTH_TEST);

    ChunksRenderer chunksRenderer = ChunksRenderer(glTextures);
    auto world = new World(255);

    bool isMouseRelative = false;

    bool running = true;
    auto lastTime = SDL_GetTicks();
    int frameCount = 0;
    int stableFrameCount = 0;

    SDL_GL_SetSwapInterval(SDL_FALSE);

    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    float aspectRatio = (float)width / (float)height;

    // Load crosshair vertices
    GLuint crosshairVAO, crosshairVBO;
    {
        glGenVertexArrays(1, &crosshairVAO);
        glGenBuffers(1, &crosshairVBO);

        glBindVertexArray(crosshairVAO);

        glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(crosshairVertices), crosshairVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    while (running) {
        globalClock.tick();

        glClearColor(0.4313, 0.6941, 1.0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) running = false;
            if (isMouseRelative)
            processMouseMotion(event, *world->player);

            if (event.type == SDL_MOUSEBUTTONDOWN)   {
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT: // Destroy
                        glm::ivec3 targetBlock;
                        glm::ivec3 hitNormal;
                        if (raymarch(
                            world->player->getPosition(),
                            world->player->camera_front,
                            1000.0f,
                            [&](const glm::ivec3 &pos) {
                                auto block = world->getBlock(Vec3i(pos));
                                return block && block->getId() != BLOCK_AIR;
                            },
                            targetBlock,
                            hitNormal
                        )) {
                            world->setBlock(BLOCK_AIR, Vec3i(targetBlock));
                        }
                    break;

                    case SDL_BUTTON_RIGHT: // Build
                        glm::ivec3 targetBlock2;
                        glm::ivec3 hitNormal2;
                        if (raymarch(
                            world->player->getPosition(),
                            world->player->camera_front,
                            1000.0f,
                            [&](const glm::ivec3 &pos) {
                                auto block = world->getBlock(Vec3i(pos));
                                return block && block->getId() != BLOCK_AIR;
                            },
                            targetBlock2,
                            hitNormal2
                        )) {
                            world->setBlock(BLOCK_PLANKS, Vec3i(targetBlock2 + hitNormal2));
                        }
                        break;
                }
            }

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

        const Uint8 *state = SDL_GetKeyboardState(nullptr);
        float camera_speed = 0.01f * globalClock.delta;
        if (state[SDL_SCANCODE_W]) world->player->moveRelative(camera_speed * world->player->camera_front);
        if (state[SDL_SCANCODE_S]) world->player->moveRelative(-(camera_speed * world->player->camera_front));
        if (state[SDL_SCANCODE_A]) world->player->moveRelative(-(glm::normalize(glm::cross(world->player->camera_front, world->player->camera_up)) * camera_speed));
        if (state[SDL_SCANCODE_D]) world->player->moveRelative(glm::normalize(glm::cross(world->player->camera_front, world->player->camera_up)) * camera_speed);

        // glBindVertexArray(vao);
        Vec3i playerPos = {static_cast<int>(world->player->getPosition().x), static_cast<int>(world->player->getPosition().y), static_cast<int>(world->player->getPosition().z)};
        chunksRenderer.renderChunks(world, shader, waterShader, playerPos);

        // Render crosshair
        {
            crosshairShader->use();
            crosshairShader->setFloat("aspectRatio", aspectRatio);
            glBindVertexArray(crosshairVAO);
            glDrawArrays(GL_LINES, 0, 4);
            glBindVertexArray(0);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (isShowDebugMenu) {
            ImGui::Begin("Debug");

            ImGui::Text("Chunks loaded: %d", world->chunks.size());
            ImGui::Text("Vertices rendered: %dk", chunksRenderer.lastCountOfTotalVerticles / 1000);
            ImGui::Text("FPS: %d", stableFrameCount);
            ImGui::Text("Seed: %d", world->seedValue);
            ImGui::Text("Position: %d, %d, %d", playerPos.x, playerPos.y, playerPos.z);

            ImGui::NewLine();

            ImGui::Checkbox("Generate new chunks", &world->isChunkGenerationEnabled);
            ImGui::Checkbox("Bake new chunks", &world->isChunkBakingEnabled);
            ImGui::Checkbox("Use single texture", &chunksRenderer.isUseSingleTexture);

            if (ImGui::Button("Halt")) {
                running = false;
            }

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);

        frameCount++;
        if (SDL_GetTicks() - lastTime > 1000) {
            lastTime = SDL_GetTicks();

            stableFrameCount = frameCount;
            frameCount = 0;
        }
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
