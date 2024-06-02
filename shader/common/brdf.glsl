#ifndef BRDF_H
#define BRDF_H

#include "constants.glsl"

float distribution_ggx(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denominator = NdotH * NdotH * (alphaSq - 1.0) + 1.0;
    denominator = pi * denominator * denominator;
    return alphaSq / max(denominator, 0.000001);
}

float smith_formula(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float HdotV, vec3 baseReflectivity) {
    return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - HdotV, 5.0);
}

#endif // BRDF_H
