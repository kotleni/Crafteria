#version 330 core

layout (location = 0) in vec3 aPos;

uniform vec3 pos;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 absolutePos = vec4(pos + (aPos), 1.0);
    gl_Position = projection * view * absolutePos;
}