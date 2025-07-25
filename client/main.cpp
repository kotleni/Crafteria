#define GL_GLEXT_PROTOTYPES

#include <array>
#define GLAD_GL_IMPLEMENTATION
#include "GL/glad.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_keycode.h>
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
#include "Player.h"
#include "World/World.h"
#include "World/BlocksIds.h"
#include "Render/ChunksRenderer.h"
#include "Math/Ray.h"
#include "utils/RuntimeConfig.h"

#include "GUI/imgui.h"
#include "GUI/imgui_impl_sdl3.h"
#include "GUI/imgui_impl_opengl3.h"
#include "GUI/imgui_internal.h"

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
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
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

bool raymarch(const glm::vec3& origin, const glm::vec3& direction, float maxLength, float stepScale,
              std::function<bool(const glm::ivec3&)> findBlock, glm::ivec3& hitBlock, glm::ivec3& prevPos) {
    glm::vec3 rayDir = glm::normalize(direction);

    for (Ray ray({origin.x, origin.y, origin.z}, rayDir);
         ray.getLength() < maxLength; ray.step(stepScale)) {
        glm::vec3 gg = {ray.getEnd().x, ray.getEnd().y, ray.getEnd().z};

        int x = floor(gg.x);
        int y = floor(gg.y);
        int z = floor(gg.z);

        if (findBlock(glm::ivec3(x, y, z))) {
            hitBlock = glm::ivec3(x, y, z);

            return true;
        }

        prevPos = glm::ivec3(x, y, z);
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

constexpr int SLOT_COUNT = 9;
constexpr int SLOT_SIZE = 46;
constexpr int SLOT_SPACING = 4;
constexpr int SLOT_PADDING = 4;
constexpr int HOTBAR_WIDTH = SLOT_COUNT * (SLOT_SIZE + SLOT_SPACING) + SLOT_SIZE + 26;
constexpr int HOTBAR_HEIGHT = SLOT_SIZE + 8;

int selectedSlot = 0;

BlockID hotbarBlocksIds[9] = {
    BLOCK_STONE,
    BLOCK_COBBLESTONE,
    BLOCK_DIRT,
    BLOCK_SNOW,
    BLOCK_SAND,
    BLOCK_GRASS,
    BLOCK_PLANKS,
    BLOCK_WATER,
    BLOCK_TORCH,
};

void renderHotbar(int screenWidth, int screenHeight, std::unordered_map<BlockID, GLuint> glTextures) {
    ImGui::SetNextWindowPos(ImVec2((screenWidth - HOTBAR_WIDTH) / 2, screenHeight - HOTBAR_HEIGHT - 10));
    ImGui::SetNextWindowSize(ImVec2(HOTBAR_WIDTH, HOTBAR_HEIGHT));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(SLOT_PADDING, SLOT_PADDING));
    ImGui::Begin("Hotbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

    for (int i = 0; i < SLOT_COUNT; i++) {
        BlockID blockId = hotbarBlocksIds[i];

        if (i > 0) ImGui::SameLine();

        ImVec4 slotColor = (i == selectedSlot) ? ImVec4(0.7f, 0.8f, 0.8f, 1.0f) : ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, slotColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.8f, 1.0f, 1.0f)); // Lighter when hovered
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.9f, 1.0f)); // Deep blue when clicked
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(SLOT_PADDING, SLOT_PADDING));

        if (ImGui::ImageButton(("##slot" + std::to_string(i)).c_str(), glTextures[blockId], ImVec2(SLOT_SIZE - SLOT_PADDING, SLOT_SIZE - SLOT_PADDING))) {
            selectedSlot = i;
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

int main() {
    RuntimeConfig runtimeConfig = RuntimeConfig();

    // Setup default runtime config values
    runtimeConfig.isEnableVsync = true;
    runtimeConfig.isMouseRelative = false;
    runtimeConfig.maxRenderingDistance = 6;
    runtimeConfig.isChunkGenerationEnabled = true;
    runtimeConfig.isChunkBakingEnabled = true;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Crafteria", 1400, 900, SDL_WINDOW_OPENGL);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::cout << std::setw(34) << std::left << "OpenGL Version: " << GLVersion.major << "." << GLVersion.minor << std::endl;
    std::cout << std::setw(34) << std::left << "OpenGL Shading Language Version: " << (char *)glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << std::setw(34) << std::left << "OpenGL Vendor:" << (char *)glGetString(GL_VENDOR) << std::endl;
    std::cout << std::setw(34) << std::left << "OpenGL Renderer:" << (char *)glGetString(GL_RENDERER) << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    bool isShowDebugMenu = true;

    // Enable debug
    // TODO: Disable for macOS (force)
    // glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageCallback(MessageCallback, 0);

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
    Shader *selectionShader = Shader::load("selection");
    Shader *floraShader = Shader::load("flora");

    // TODO
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND);

    glEnable(GL_DEPTH_TEST);

    ChunksRenderer chunksRenderer = ChunksRenderer(glTextures, &runtimeConfig);
    auto world = new World(255, &runtimeConfig);

    bool running = true;
    auto lastTime = SDL_GetTicks();
    int frameCount = 0;
    int stableFrameCount = 0;
    int fpsRangeCount = 16;
    int peakFps = 0;
    std::vector<float> fpsRanges = std::vector<float>(fpsRangeCount);

    SDL_GL_SetSwapInterval(runtimeConfig.isEnableVsync);

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

    std::vector<SDL_Keycode> pressedKeys = std::vector<SDL_Keycode>(8);

    while (running) {
        globalClock.tick();

        glClearColor(0.4313, 0.6941, 1.0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (!runtimeConfig.isMouseRelative)
                ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) running = false;
            if (runtimeConfig.isMouseRelative)
            processMouseMotion(event, *world->player);

            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (runtimeConfig.isMouseRelative)
                    SDL_WarpMouseInWindow(window, width / 2, height / 2);
            } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                if (event.wheel.y > 0) {
                    selectedSlot = (selectedSlot - 1 + SLOT_COUNT) % SLOT_COUNT; // Scroll up
                } else if (event.wheel.y < 0) {
                    selectedSlot = (selectedSlot + 1) % SLOT_COUNT; // Scroll down
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)   {
                switch (event.button.button) {
                    case SDL_BUTTON_LEFT: // Destroy
                        glm::ivec3 targetBlock;
                        glm::ivec3 prevPos;
                        if (raymarch(
                            world->player->getPosition(),
                            world->player->camera_front,
                            36.0f,
                            0.05f,
                            [&](const glm::ivec3 &pos) {
                                auto block = world->getBlock(Vec3i(pos));
                                return block && block->isSolid();
                            },
                            targetBlock,
                            prevPos
                        )) {
                            world->setBlock(BLOCK_AIR, Vec3i(targetBlock));
                        }
                    break;

                    case SDL_BUTTON_RIGHT: // Build
                        glm::ivec3 targetBlock2;
                        glm::ivec3 prevPos2;
                        if (raymarch(
                            world->player->getPosition(),
                            world->player->camera_front,
                            36.0f,
                            0.05f,
                            [&](const glm::ivec3 &pos) {
                                auto block = world->getBlock(Vec3i(pos));
                                return block && block->isSolid();
                            },
                            targetBlock2,
                            prevPos2
                        )) {
                            world->setBlock(hotbarBlocksIds[selectedSlot], Vec3i(prevPos2));
                        }
                        break;
                }
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (std::ranges::find(pressedKeys, event.key.key) == pressedKeys.end()) {
                    pressedKeys.push_back(event.key.key);
                }

                switch (event.key.key) {
                    case SDLK_F3:
                        runtimeConfig.isMouseRelative = !runtimeConfig.isMouseRelative;
                    SDL_SetWindowRelativeMouseMode(window, runtimeConfig.isMouseRelative);
                    break;
                }
            } else if (event.type == SDL_EVENT_KEY_UP) {
                pressedKeys.erase(std::ranges::remove(pressedKeys, event.key.key).begin(), pressedKeys.end());
            }
        }

        float camera_speed = 0.01f * globalClock.delta;
        for (SDL_Keycode key : pressedKeys) {
            switch (key) {
                case SDLK_W:
                    world->player->moveRelative(camera_speed * world->player->camera_front);
                break;
                case SDLK_S:
                    world->player->moveRelative(-(camera_speed * world->player->camera_front));
                break;
                case SDLK_A:
                    world->player->moveRelative(-(glm::normalize(glm::cross(world->player->camera_front, world->player->camera_up)) * camera_speed));
                break;
                case SDLK_D:
                    world->player->moveRelative(glm::normalize(glm::cross(world->player->camera_front, world->player->camera_up)) * camera_speed);
                break;
            }
        }

        glm::ivec3 targetBlock2;
        glm::ivec3 hitNormal2;
        if (raymarch(
            world->player->getPosition(),
            world->player->camera_front,
            36.0f,
            0.5f,
            [&](const glm::ivec3 &pos) {
                auto block = world->getBlock(Vec3i(pos));
                return block && block->isSolid();
            },
            targetBlock2,
            hitNormal2
        )) {
            chunksRenderer.targetBlock = Vec3i(targetBlock2);
        }

        // glBindVertexArray(vao);
        Vec3i playerPos = {static_cast<int>(world->player->getPosition().x), static_cast<int>(world->player->getPosition().y), static_cast<int>(world->player->getPosition().z)};
        chunksRenderer.renderChunks(world, shader, waterShader, selectionShader, floraShader, playerPos);

        // Render crosshair
        {
            crosshairShader->use();
            crosshairShader->setFloat("aspectRatio", aspectRatio);
            glBindVertexArray(crosshairVAO);
            glLineWidth(2.0f);
            glDrawArrays(GL_LINES, 0, 4);
            glBindVertexArray(0);
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        renderHotbar(width, height, glTextures);

        if (isShowDebugMenu) {
            ImGui::Begin("Menu", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::BeginTabBar("#tabs");
            if (ImGui::BeginTabItem("Debug")) {
                ImGui::Text("Chunks loaded: %d", world->chunks.size());
                ImGui::Text("Polygons rendered: %dk", (chunksRenderer.lastCountOfTotalVertices / 3) / 1000 /* (vertices / VERTICES_PER_POLYGON) / UNITS_TO_THOUSANDS */);
                ImGui::Text("FPS: %d", stableFrameCount);
                ImGui::Text("Seed: %d", world->seedValue);
                ImGui::Text("Position: %d, %d, %d", playerPos.x, playerPos.y, playerPos.z);
                ImGui::Text("Look at: %.2f, %.2f, %.2f", world->player->camera_front.x, world->player->camera_front.y, world->player->camera_front.z);

                ImGui::PlotLines("FPS", fpsRanges.data(), fpsRanges.size(), 0, 0, 0, std::max(60, peakFps), ImVec2(0, 64));

                ImGui::Separator();

                ImGui::Checkbox("Generate new chunks", &runtimeConfig.isChunkGenerationEnabled);
                ImGui::Checkbox("Bake new chunks", &runtimeConfig.isChunkBakingEnabled);

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Settings")) {
                if (ImGui::Checkbox("VSync", &runtimeConfig.isEnableVsync)) {
                    SDL_GL_SetSwapInterval(runtimeConfig.isEnableVsync);
                }

                if (ImGui::SliderInt("Render distance", &runtimeConfig.maxRenderingDistance, 2, 32)) {

                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Experiments")) {
                if (ImGui::Button("Remove all blocks in chunk")) {
                    Vec3i playerChunkPos = {
                        static_cast<int>(world->player->getPosition().x / CHUNK_SIZE_XZ), 0,
                        static_cast<int>(world->player->getPosition().z / CHUNK_SIZE_XZ)
                    };

                    runtimeConfig.isChunkBakingEnabled = false;
                    for (int x = 0; x < CHUNK_SIZE_XZ; x++) {
                        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
                            for (int z = 0; z < CHUNK_SIZE_XZ; z++) {
                                    world->setBlock(BLOCK_AIR, Vec3i(playerChunkPos.x + x, playerChunkPos.y + y, playerChunkPos.z + z));
                            }
                        }
                    }
                    runtimeConfig.isChunkBakingEnabled = true;
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();

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

            if (peakFps < stableFrameCount)
                peakFps = stableFrameCount;

            fpsRanges.insert(fpsRanges.end(), stableFrameCount);
            if (fpsRanges.size() > fpsRangeCount) {
                fpsRanges.erase(fpsRanges.begin() - 1, fpsRanges.begin() - 1);
            }
        }
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
