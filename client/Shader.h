#ifndef H_SHADER
#define H_SHADER

#include "GL/glad.h"
#include <GL/gl.h>

#include <SDL3/SDL_opengl.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

class Shader {
public:
    GLuint program;

    Shader(GLint programId);
    void use();

    void setMat4(const char* key, glm::mat4 mat);
    void setVec4(const char* key, glm::vec4 vec);
    void setVec3(const char* key, glm::vec3 vec);
    void setVec2(const char* key, glm::vec2 vec);
    void setFloat(const char* key, float value);
    void setBool(const char* key, bool value);
    void setInt(const char* key, int value);

    static Shader *load(std::string file);
};

#endif