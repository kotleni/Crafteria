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

glm::vec3 camera_front(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up(0.0f, 1.0f, 0.0f);

/**
 * Cached chunks renderer
 */
class ChunksRenderer {
    glm::mat4 projection = glm::perspective(glm::radians(75.0f), 800.0f / 600.0f, 0.1f, 1000.0f);
    glm::vec3 lightPos = glm::vec3(10.0f, 10.0f, 10.0f);
    glm::mat4 mat4One = glm::mat4(1.0f);

    std::unordered_map<BlockID, GLuint> glTextures;
public:
    int lastCountOfTotalVerticles = 0;

    ChunksRenderer(std::unordered_map<BlockID, GLuint> glTextures): glTextures(glTextures) { }

    void renderChunks(World* world, Shader *shader, Shader *waterShader, Vec3i playerPos) {
        lastCountOfTotalVerticles = 0;

        // Copy chunks array
        std::vector<Chunk *> chunks = world->chunks;

        glm::mat4 view = glm::lookAt(world->player->position, world->player->position + camera_front, camera_up);
        glm::vec3 pos;

        shader->use();
        shader->setMat4("view", view);
        shader->setMat4("projection", projection);
        shader->setVec3("lightPos", this->lightPos);
        shader->setVec3("viewPos", world->player->position);

        glDisable(GL_BLEND);

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
            BakedChunk *bakedChunk = chunk->bakedChunk;

            // Chunk is not baked yet?
            if (bakedChunk == nullptr) continue;

            pos.x = chunk->position.x;
            pos.y = chunk->position.y;
            pos.z = chunk->position.z;
            pos *= CHUNK_SIZE_XZ;

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

                glBindTexture(GL_TEXTURE_2D, this->glTextures[part.blockID]);
                glDrawElements(GL_TRIANGLES, part.indices.size(), GL_UNSIGNED_INT, nullptr);
                lastCountOfTotalVerticles += part.vertices.size() / 9; // Verticles count
            }
        }

        // Copy actual chunks array
        chunks = world->chunks;

        waterShader->use();
        waterShader->setMat4("view", view);
        waterShader->setMat4("projection", projection);
        waterShader->setVec3("lightPos", this->lightPos);
        waterShader->setVec3("viewPos", world->player->position);
        waterShader->setFloat("time", SDL_GetTicks() / 1000.0f);

        glEnable(GL_BLEND);

        // Draw all liquid
        for (const auto &chunk: chunks) {
            double distance = (chunk->position * CHUNK_SIZE_XZ).distanceTo({playerPos.x, 0, playerPos.z});
            if (distance > CHUNK_RENDERING_DISTANCE_IN_BLOCKS) {
                continue;
            }
            BakedChunk *bakedChunk = chunk->bakedChunk;

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

    // TODO
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND);

    glEnable(GL_DEPTH_TEST);

    ChunksRenderer chunksRenderer = ChunksRenderer(glTextures);
    auto world = new World(SDL_GetTicks());

    bool isMouseRelative = false;

    bool running = true;
    auto lastTime = SDL_GetTicks();
    int frameCount = 0;
    int stableFrameCount = 0;

    SDL_GL_SetSwapInterval(SDL_FALSE);

    while (running) {
        globalClock.tick();

        glClearColor(0.4313, 0.6941, 1.0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) running = false;
            if (isMouseRelative)
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

        const Uint8 *state = SDL_GetKeyboardState(nullptr);
        float camera_speed = 0.01f * globalClock.delta;
        if (state[SDL_SCANCODE_W]) world->player->position += camera_speed * camera_front;
        if (state[SDL_SCANCODE_S]) world->player->position -= camera_speed * camera_front;
        if (state[SDL_SCANCODE_A]) world->player->position -= glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;
        if (state[SDL_SCANCODE_D]) world->player->position += glm::normalize(glm::cross(camera_front, camera_up)) * camera_speed;

        // glBindVertexArray(vao);
        Vec3i playerPos = {static_cast<int>(world->player->position.x), static_cast<int>(world->player->position.y), static_cast<int>(world->player->position.z)};
        chunksRenderer.renderChunks(world, shader, waterShader, playerPos);

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
