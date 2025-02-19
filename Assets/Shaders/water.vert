#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aLight;

out vec2 TexCoord;
out vec3 Normal;
out float vLight;
out float viewDistance;

uniform vec3 pos;
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

    vLight = aLight;

    vec4 absolutePos = vec4(pos + aPos, 1.0);
    gl_Position = projection * view * absolutePos;

    vec4 viewPos = view * absolutePos;
    viewDistance = length(viewPos.xyz);
}