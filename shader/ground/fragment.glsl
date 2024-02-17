#version 450

layout(location = 0) in vec4 fragWorldPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragViewPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

vec3 pattern_pixel(ivec2 coord, int size, float mul) {
    int halfSize = size / 2;
    int oneEight = size / 16;
    ivec2 value = coord / halfSize;

    ivec2 value2 = coord / oneEight;
    float minor;
    if ((value2.x + value2.y) % 2 == 0) {
        minor = 0.05;
    } else {
        minor = -0.05;
    }

    float major;
    if ((value.x + value.y) % 2 == 0) {
        major = 0.1;
    } else {
        major = -0.1;
    }

    return vec3(0.8 + mul*(minor + major));
}

vec2 sample_pattern(vec2 coord, float mul) {
    ivec2 f1 = ivec2(floor(coord));
    ivec2 f2 = ivec2(floor(10*coord));

    float metallic = 0;

    float minor;
    if ((f1.x + f1.y) % 2 == 0) {
        metallic = mul * 0.8;
        minor = 0.05;
    } else {
        minor = -0.05;
    }

    float major;
    if ((f2.x + f2.y) % 2 == 0) {
        major = 0.1;
    } else {
        major = -0.1;
    }

    return vec2(0.8 + mul * (major + minor), metallic);
}

void main() {
    vec3 pos = fragViewPosition.xyz / fragViewPosition.w;
    vec2 pattern = sample_pattern(0.2 * fragWorldPosition.xy, 6.0 / abs(pos.z));
    outColor = vec4(vec3(pattern.x), 1.0);
    outPosition = vec4(fragViewPosition.xyz, pattern.y);
    outNormal = vec4(normalize(fragNormal), 1.0);
}