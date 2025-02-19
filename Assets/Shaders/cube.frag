#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in float vLight;
in float viewDistance;

uniform sampler2D ourTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    vec4 rgba = texture(ourTexture, TexCoord);
    vec3 color = vec3(rgba.x, rgba.y, rgba.z);

    float fogMaxDist = 70.0;
    float fogMinDist = 12.0;
    vec3 fogColor = vec3(0.623, 0.734, 0.785);

    // Calculate fog
    float fogFactor = (fogMaxDist - viewDistance) /
    (fogMaxDist - fogMinDist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 result = mix(color, fogColor, 1.0 - fogFactor) * vLight;

    FragColor = vec4(result, 1.0);
}
