#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in float vLight;

uniform sampler2D ourTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    vec4 rgba = texture(ourTexture, TexCoord);
    vec3 color = vec3(rgba.x, rgba.y, rgba.z);

    vec3 result = color * vLight; // (ambient + diff) * color;

    FragColor = vec4(result, 1.0);
}
