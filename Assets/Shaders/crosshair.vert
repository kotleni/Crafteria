#version 330 core
layout (location = 0) in vec2 aPos;
uniform float aspectRatio;

void main() {
    gl_Position = vec4(aPos.x / aspectRatio, aPos.y, 0.0, 1.0);
}