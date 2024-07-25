#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texPosition;
layout(binding = 2) uniform sampler2D texNormal;
layout(binding = 3) uniform sampler2D texAmbientOcclusion;
layout(binding = 4) uniform sampler2D texShadowMaps[];

layout(binding = 5) uniform ShadingData {
    mat4 viewMat;
    mat4 shadowMapMats[3];
} sd;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBloom;

layout(push_constant) uniform Constants
{
    vec3 lightPosition;
    bool enableSSAO;
} pc;

#include "../common/brdf.glsl"
#include "../common/constants.glsl"
#include "../common/shadow_map.glsl"

const float ambient = 0.5;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);

vec3 calculate_light(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 baseReflectivity, vec3 albedo, float roughness, float metallic)
{
    vec3 halfpoint = normalize(viewDir + lightDir);
    float attenuation = 4.0;
    vec3 radience = vec3(1, 1, 1) * attenuation;

    float NdotV = max(dot(normal, viewDir), 0.00000001);
    float NdotL = max(dot(normal, lightDir), 0.00000001);
    float HdotV = max(dot(halfpoint, viewDir), 0);
    float NdotH = max(dot(normal, halfpoint), 0);

    float dist = distribution_ggx(NdotH, roughness);
    float geo = smith_formula(NdotV, NdotL, roughness);
    vec3 fresnel = fresnel_schlick(HdotV, baseReflectivity);

    vec3 specular = dist * geo * fresnel;
    specular /= 4.0 * NdotV * NdotL;

    vec3 kD = vec3(1.0) - fresnel;
    kD *= 1.0 - metallic;

    return (kD * albedo / pi + specular) * radience * NdotL;
}

void main() {
    vec3 normal = texture(texNormal, fragTexCoord).rgb;
    vec4 texColorSample = texture(texColor, fragTexCoord);
    if (normal == vec3(0.0, 0.0, 0.0)) {
        outColor = vec4(texColorSample.rgb, 1.0);
        outBloom = vec4(0.0);
        return;
    }
    normal = normalize(normal);

    vec4 texPositionSample = texture(texPosition, fragTexCoord);
    vec3 position = texPositionSample.xyz;

    vec4 shadowUV = biasMat * sd.shadowMapMats[0] * vec4(position, 1.0);
    shadowUV /= shadowUV.w;


    float shadow;

    if (shadow_map_bound_test(shadowUV)) {
        shadow = shadow_map_test_pcr(texShadowMaps[0], shadowUV, 0.006);
//        texColorSample.rgb *= vec3(1.0, 0.1, 0.1);
    } else  {
        shadowUV = biasMat * sd.shadowMapMats[1] * vec4(position, 1.0);
        shadowUV /= shadowUV.w;
        if (shadow_map_bound_test(shadowUV)) {
            shadow = shadow_map_test_pcr(texShadowMaps[1], shadowUV, 0.006);
//            texColorSample.rgb *= vec3(0.1, 1.0, 0.1);
        } else {
            shadowUV = biasMat * sd.shadowMapMats[2] * vec4(position, 1.0);
            shadowUV /= shadowUV.w;
            shadow = shadow_map_test_pcr(texShadowMaps[2], shadowUV, 0.006);
//            texColorSample.rgb *= vec3(0.1, 0.1, 1.0);
        }
    }

    float ambientValue = ambient;
    if (pc.enableSSAO) {
        ambientValue *= pow(texture(texAmbientOcclusion, fragTexCoord).r, 1.5);
    }

    vec3 albedo = texColorSample.rgb;

    const float roughness = texColorSample.w;
    const float metallic = texPositionSample.w;

    vec3 viewDir = normalize(-position);
    vec3 baseReflectivity = mix(vec3(0.04), albedo, metallic);

    ShadingInfo shadingInfo;
    shadingInfo.normal = normal;
    shadingInfo.viewDir = viewDir;
    shadingInfo.position = position;
    shadingInfo.baseReflectivity = baseReflectivity;
    shadingInfo.albedo = albedo;
    shadingInfo.roughness = roughness;
    shadingInfo.metallic = metallic;

    vec3 Lo = vec3(0);

    vec3 lightDir = normalize(mat3(sd.viewMat) * vec3(-1, 0.0, -0.4));
    Lo += calculate_directional_light(shadingInfo, lightDir);

    Lo += calculate_point_light(shadingInfo, pc.lightPosition, vec3(1, 0.6, 0.05), 24.0);

    vec3 ambient = albedo * ambientValue;
    vec3 color = 0.5 * ambient + 1.2 * shadow * Lo;

    float luminance = dot(color, vec3(0.2125, 0.7153, 0.07121));
    color = mix(vec3(luminance), color, 1.25);
    outBloom = vec4(0.0);

    outColor = vec4(color, 1.0);
}
