#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aLight;

out vec2 TexCoord;
out vec3 FragPos;
out float vLight;

uniform vec3 pos;
uniform mat4 view;
uniform mat4 projection;

void main() {
    TexCoord = aTexCoord;
    vLight = aLight;

    vec4 absolutePos = vec4(pos + aPos, 1.0);
    gl_Position = projection * view * absolutePos;
}