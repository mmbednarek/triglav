#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 passedPosition;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

const vec3 lightPosition = vec3(0, 0, 0);
const vec4 black = vec4(0, 0, 0, 1);

void main() {
    vec4 texPixel = texture(texSampler, fragTexCoord);
    float dist = distance(passedPosition, lightPosition);
    float lightStrength = clamp(dist / 2.3, 0.0, 1.0);
    float lightCutoff = dist < 2.3 ? 0 : lightStrength;
    outColor = mix(texPixel, black, lightStrength);
}
