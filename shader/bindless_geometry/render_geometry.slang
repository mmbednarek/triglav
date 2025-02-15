import "../common/common.slang";
import material.material;

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

material::FSOutput fs_main(material::FSInput fsInput)
{
    MaterialProperties properties = MaterialPropertiesBuffer[fsInput.materialID];

    let albedoColor = material::MaterialTextures[properties.albedoTextureID].Sample(fsInput.texCoord);

#if MT_USE_NORMAL_MAP
    let tangentSpaceMat = float3x3(fsInput.tangent, fsInput.bitangent, fsInput.normal);
    let sampledNormal = 2 * material::MaterialTextures[properties.normalTextureID].Sample(fsInput.texCoord).xyz - float3(1, 1, 1);
    let normalVec = normalize(mul(tangentSpaceMat, sampledNormal));
#else
    let normalVec = normalize(fsInput.normal);
#endif

#if MT_USE_METALIC_MAP
    let metallic = material::MaterialTextures[properties.metallicTextureID].Sample(fsInput.texCoord).x;
#else
    let metallic = properties.metallic;
#endif
#if MT_USE_ROUGHNESS_MAP
    let roughness = material::MaterialTextures[properties.roughnessTextureID].Sample(fsInput.texCoord).x;
#else
    let roughness = properties.roughness;
#endif

    material::FSOutput output;
    output.color = float4(albedoColor.xyz, roughness);
    output.normal = float4(normalVec, 1.0);
    output.position = float4(fsInput.viewSpacePosition.xyz, metallic);
    return output;
}
