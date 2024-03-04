#version 450

layout(location = 0) in vec4 fragWorldPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragViewPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

layout(binding = 1) uniform sampler2D texTile;

void main() {
    vec3 pos = fragViewPosition.xyz / fragViewPosition.w;
    float dist = clamp(2.0 / sqrt(abs(pos.z)) - 0.1, 0.0, 1.0);
    float color = mix(0.7, texture(texTile, 0.2 * fragWorldPosition.xy).r, dist);
    outColor = vec4(vec3(color), 1.0);

    outPosition = vec4(fragViewPosition.xyz, color - 0.3);
    outNormal = vec4(normalize(fragNormal), 1.0);
}