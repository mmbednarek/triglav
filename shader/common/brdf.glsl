#ifndef BRDF_H
#define BRDF_H

#include "constants.glsl"

float distribution_ggx(float NdotH, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denominator = NdotH * NdotH * (alphaSq - 1.0) + 1.0;
    denominator = pi * denominator * denominator;
    return alphaSq / max(denominator, 0.000001);
}

float smith_formula(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float HdotV, vec3 baseReflectivity)
{
    return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - HdotV, 5.0);
}

struct ShadingInfo
{
    vec3 normal;
    vec3 viewDir;
    vec3 position;
    vec3 baseReflectivity;
    vec3 albedo;
    float roughness;
    float metallic;
};

vec3 calculate_directional_light(ShadingInfo info, vec3 lightDir)
{
    vec3 halfpoint = normalize(info.viewDir + lightDir);
    float attenuation = 4.0;
    vec3 radience = vec3(1, 1, 1) * attenuation;

    float NdotV = max(dot(info.normal, info.viewDir), 0.00000001);
    float NdotL = max(dot(info.normal, lightDir), 0.00000001);
    float HdotV = max(dot(halfpoint, info.viewDir), 0);
    float NdotH = max(dot(info.normal, halfpoint), 0);

    float dist = distribution_ggx(NdotH, info.roughness);
    float geo = smith_formula(NdotV, NdotL, info.roughness);
    vec3 fresnel = fresnel_schlick(HdotV, info.baseReflectivity);

    vec3 specular = dist * geo * fresnel;
    specular /= 4.0 * NdotV * NdotL;

    vec3 kD = vec3(1.0) - fresnel;
    kD *= 1.0 - info.metallic;

    return (kD * info.albedo / pi + specular) * radience * NdotL;
}

vec3 calculate_point_light(ShadingInfo info, vec3 lightPosition, vec3 lightColor, float range)
{
    vec3 posDiff = lightPosition - info.position;

    float lightDist = length(posDiff);
    if (lightDist > range) {
        return vec3(0);
    }

    vec3 lightDir = normalize(posDiff);
    return pow(1 - lightDist/range, 2) * lightColor * calculate_directional_light(info, lightDir);
}

#endif // BRDF_H
