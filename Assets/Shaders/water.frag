#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in float vLight;

uniform sampler2D ourTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 worldPos;
uniform float time;

void main() {
    vec4 rgba = texture(ourTexture, TexCoord);
    vec3 color = vec3(rgba.x, rgba.y, rgba.z) * vLight;
    FragColor = vec4(color, 0.5);
}
