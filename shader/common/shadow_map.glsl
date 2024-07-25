#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

const int g_pcrFilterRange = 2;
const int g_pcrFilterKernelCount = (2*g_pcrFilterRange + 1) * (2*g_pcrFilterRange + 1);

bool shadow_map_bound_test(vec4 shadowCoord)
{
    return shadowCoord.z > -0.99 && shadowCoord.z < 0.99 &&
           shadowCoord.y > 0.01 && shadowCoord.y < 0.99 &&
           shadowCoord.x > 0.01 && shadowCoord.x < 0.99;
}

float shadow_map_test(sampler2D shadowMap, vec4 shadowCoord, vec2 off, float bias)
{
    float shadow = 1.0;

    if (shadow_map_bound_test(shadowCoord)) {
        float dist = texture(shadowMap, shadowCoord.xy + off).r;

        if (shadowCoord.w > 0.0 && dist < shadowCoord.z - bias) {
            shadow = 0;
        }
    }

    return shadow;
}

float shadow_map_test_pcr(sampler2D shadowMap, vec4 shadowCoord, float bias)
{
    ivec2 texDim = textureSize(shadowMap, 0);
    float scale = 1.5;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;

    for (int x = -g_pcrFilterRange; x <= g_pcrFilterRange; x++) {
        for (int y = -g_pcrFilterRange; y <= g_pcrFilterRange; y++) {
            shadowFactor += shadow_map_test(shadowMap, shadowCoord, vec2(dx*x, dy*y), bias);
        }
    }

    return shadowFactor / g_pcrFilterKernelCount;
}

#endif // SHADOW_MAP_H
