implementing material;

namespace material
{
    struct AlbedoNormalTextureProperties
    {
        uint albedoTextureID;
        uint normalTextureID;
        float roughness;
        float metalic;
    };

    [[vk::binding(5)]]
    uniform StructuredBuffer<AlbedoNormalTextureProperties> AlbedoNormalTexturePropertiesArray;

    FSOutput process_albedo_normal_texture(FSInput input, const uint matInstanceID)
    {
        AlbedoNormalTextureProperties props = AlbedoNormalTexturePropertiesArray[matInstanceID];

        let albedoColor = MaterialTextures[props.albedoTextureID].Sample(input.texCoord);

        let tangentSpaceMat = float3x3(input.tangent, input.bitangent, input.normal);
        let normalVec = 2 * MaterialTextures[props.normalTextureID].Sample(input.texCoord).xyz - float3(1, 1, 1);

        FSOutput output;
        output.color = float4(albedoColor.xyz, props.roughness);
        output.normal = float4(normalize(mul(tangentSpaceMat, normalVec)), 1.0);
        output.position = float4(input.viewSpacePosition.xyz, props.metalic);

        return output;
    }
}

