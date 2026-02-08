import triglav.core;
import triglav.material;

typealias FSInput = triglav::material::FSInput;
typealias FSOutput = triglav::material::FSOutput;

// MT_USE_NORMAL_MAP
// MT_USE_ROUGHNESS_MAP
// MT_USE_METALIC_MAP

struct MaterialProperties
{
    uint albedoTextureID;

#if MT_USE_NORMAL_MAP
    uint normalTextureID;
#endif

#if MT_USE_ROUGHNESS_MAP
    uint roughnessTextureID;
#else
    float roughness;
#endif

#if MT_USE_METALIC_MAP
    uint metallicTextureID;
#else
    float metallic;
#endif
};

[[vk::binding(3)]]
uniform StructuredBuffer<MaterialProperties> MaterialPropertiesBuffer;

FSOutput fs_main(FSInput fsInput)
{
    MaterialProperties properties = MaterialPropertiesBuffer[fsInput.materialID];

    let albedoColor = triglav::material::MaterialTextures[properties.albedoTextureID].Sample(fsInput.texCoord);

#if MT_USE_NORMAL_MAP
    let tangentSpaceMat = float3x3(normalize(fsInput.tangent), normalize(fsInput.bitangent), normalize(fsInput.normal));
    let sampledNormal = 2 * triglav::material::MaterialTextures[properties.normalTextureID].Sample(fsInput.texCoord).xyz - float3(1, 1, 1);
    let normalVec = normalize(mul(sampledNormal, tangentSpaceMat));
#else
    let normalVec = normalize(fsInput.normal);
#endif

#if MT_USE_METALIC_MAP
    let metallic = triglav::material::MaterialTextures[properties.metallicTextureID].Sample(fsInput.texCoord).x;
#else
    let metallic = properties.metallic;
#endif
#if MT_USE_ROUGHNESS_MAP
    let roughness = triglav::material::MaterialTextures[properties.roughnessTextureID].Sample(fsInput.texCoord).x;
#else
    let roughness = properties.roughness;
#endif

    FSOutput output;
    output.color = float4(albedoColor.xyz, roughness);
    output.normal = float4(normalVec, 1.0);
    output.position = float4(fsInput.viewSpacePosition.xyz, metallic);
    return output;
}
