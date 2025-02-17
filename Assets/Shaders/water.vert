#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aLight;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out float vLight;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main() {
    TexCoord = aTexCoord;

    float waveStrength = 0.1;
    float waveSpeed = 1.1;
    float waveFrequency = 2.0;

    vec3 modifiedPos = aPos;
    modifiedPos.y -= waveStrength;
    modifiedPos.y += waveStrength * sin(waveFrequency * aPos.x + time * waveSpeed);

    FragPos = vec3(model * vec4(modifiedPos, 1.0));

    vLight = aLight;

    // Convert normal to world space
    Normal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}