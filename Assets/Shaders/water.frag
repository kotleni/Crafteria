#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D ourTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 worldPos;
uniform float time;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 ambient = 0.3 * vec3(1.0, 1.0, 1.0);

    vec4 rgba = texture(ourTexture, TexCoord);
    vec3 color = vec3(rgba.x, rgba.y, rgba.z);

    vec3 toCameraVector = viewPos - worldPos;
    vec3 viewVector = normalize(toCameraVector);
    vec3 faceNormal = vec3(0.0, 1.0, 0.0);
    float fresnel = dot(viewVector, faceNormal);

    vec3 result = color * fresnel; // (ambient + diff) * color;

    FragColor = vec4(result, 0.5);
}
