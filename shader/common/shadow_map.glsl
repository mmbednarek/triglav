#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

const int g_pcrFilterRange = 2;
const int g_pcrFilterKernelCount = (2*g_pcrFilterRange + 1) * (2*g_pcrFilterRange + 1);

float shadow_map_test(sampler2D shadowMap, vec4 shadowCoord, vec2 off)
{
    float shadow = 1.0;

    if (shadowCoord.z >= -1.0 && shadowCoord.z < 1.0 &&
        shadowCoord.y > 0.0 && shadowCoord.y < 1.0 &&
        shadowCoord.x > 0.0 && shadowCoord.x < 1.0) {
        float dist = texture(shadowMap, shadowCoord.xy + off).r;

        if (shadowCoord.w > 0.0 && dist < shadowCoord.z - 0.005) {
            shadow = 0;
        }
    }

    return shadow;
}

float shadow_map_test_pcr(sampler2D shadowMap, vec4 shadowCoord)
{
    ivec2 texDim = textureSize(shadowMap, 0);
    float scale = 1.5;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;

    for (int x = -g_pcrFilterRange; x <= g_pcrFilterRange; x++) {
        for (int y = -g_pcrFilterRange; y <= g_pcrFilterRange; y++) {
            shadowFactor += shadow_map_test(shadowMap, shadowCoord, vec2(dx*x, dy*y));
        }
    }

    return shadowFactor / g_pcrFilterKernelCount;
}

#endif // SHADOW_MAP_H
