#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in float vLight;
in float viewDistance;

uniform sampler2D ourTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 worldPos;
uniform float time;

void main() {
    vec4 rgba = texture(ourTexture, TexCoord);
    vec3 color = vec3(rgba.x, rgba.y, rgba.z) * vLight;
    vec4 realColor = vec4(color, 0.5);

    float fogMaxDist = 70.0;
    float fogMinDist = 32.0;
    vec4 fogColor = vec4(0.623, 0.734, 0.785, 1.0);

    // Calculate fog
    float fogFactor = (fogMaxDist - viewDistance) /
    (fogMaxDist - fogMinDist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    FragColor = mix(realColor, fogColor, 1.0 - fogFactor);
}
