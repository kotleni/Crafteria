#define GL_GLEXT_PROTOTYPES

#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

const char *vertex_shader_src =
    "#version 130\n"
    "in vec3 i_position;\n"
    "in vec4 i_color;\n"
    "out vec4 v_color;\n"
    "uniform mat4 u_transform;\n"
    "void main() {\n"
    "    v_color = i_color;\n"
    "    gl_Position = u_transform * vec4(i_position, 1.0);\n"
    "}\n";

const char *fragment_shader_src =
    "#version 130\n"
    "in vec4 v_color;\n"
    "out vec4 o_color;\n"
    "void main() {\n"
    "    o_color = v_color;\n"
    "}\n";

struct Cube {
    glm::vec3 position;
};

const int CHUNK_SIZE = 16;
std::vector<Cube> generateChunk() {
    std::vector<Cube> chunk;
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int z = 0; z < CHUNK_SIZE; ++z) {
                chunk.push_back({glm::vec3(x, y, z)});
            }
        }
    }
    return chunk;
}

GLfloat cube_vertices[] = {
    -0.5f, -0.5f, -0.5f, 1, 0, 0, 1,
     0.5f, -0.5f, -0.5f, 0, 1, 0, 1,
     0.5f,  0.5f, -0.5f, 0, 0, 1, 1,
    -0.5f,  0.5f, -0.5f, 1, 1, 1, 1,

    -0.5f, -0.5f,  0.5f, 1, 0, 1, 1,
     0.5f, -0.5f,  0.5f, 0, 1, 1, 1,
     0.5f,  0.5f,  0.5f, 1, 1, 0, 1,
    -0.5f,  0.5f,  0.5f, 0, 0, 0, 1
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

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Chunk Rendering", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &vertex_shader_src, NULL);
    glCompileShader(vs);
    glShaderSource(fs, 1, &fragment_shader_src, NULL);
    glCompileShader(fs);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glUseProgram(program);

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glEnable(GL_DEPTH_TEST);
    
    std::vector<Cube> chunk = generateChunk();
    glm::vec3 camera_pos(8.0f, 8.0f, 20.0f);
    glm::vec3 camera_front(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up(0.0f, 1.0f, 0.0f);
    
    SDL_SetRelativeMouseMode(SDL_TRUE);

    bool running = true;
    while (running) {
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
        glm::mat4 u_transform = projection * view;
        glUniformMatrix4fv(glGetUniformLocation(program, "u_transform"), 1, GL_FALSE, &u_transform[0][0]);

        glBindVertexArray(vao);
        for (const auto &cube : chunk) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cube.position);
            glm::mat4 final_transform = u_transform * model;
            glUniformMatrix4fv(glGetUniformLocation(program, "u_transform"), 1, GL_FALSE, &final_transform[0][0]);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        SDL_GL_SwapWindow(window);
    }
    SDL_Quit();
    return 0;
}
