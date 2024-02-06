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

vec3 sample_pattern(vec2 coord, float mul) {
    int size = 256;
    vec2 coord2 = size * mod(coord, vec2(1.0));
    vec2 modulo = mod(coord2, vec2(1.0));
    vec2 moduloInv = 1.0 - modulo;
    ivec2 intCoord = ivec2(coord2 - modulo);

    vec3 pixelAA = pattern_pixel(intCoord, size, mul);
    vec3 pixelAB = pattern_pixel(intCoord + ivec2(0, 1), size, mul);
    vec3 pixelBA = pattern_pixel(intCoord + ivec2(1, 0), size, mul);
    vec3 pixelBB = pattern_pixel(intCoord + ivec2(1, 1), size, mul);

    return pixelAA * moduloInv.x * moduloInv.y +
        pixelAB * moduloInv.x * modulo.y +
        pixelBA * modulo.x * moduloInv.y +
        pixelBB * modulo.x * modulo.y;
}

void main() {
    vec3 pos = fragViewPosition.xyz / fragViewPosition.w;
    vec3 squareColor = sample_pattern(0.2 * fragWorldPosition.xy, 8.0 / abs(pos.z));

    outColor = vec4(squareColor, 1.0);
    outPosition = vec4(fragViewPosition.xyz, 0.0);
    outNormal = vec4(normalize(fragNormal), 1.0);
}