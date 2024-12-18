
implementing material;

namespace material
{
    struct AllTextureProperties
    {
        uint albedoTextureID;
        uint normalTextureID;
        uint roughnessTextureID;
        uint metalicTextureID;
    };

    [[vk::binding(6)]]
    uniform StructuredBuffer<AllTextureProperties> AllTexturePropertiesArray;

    FSOutput process_all_texture(FSInput input, const uint matInstanceID)
    {
        AllTextureProperties props = AllTexturePropertiesArray[matInstanceID];

        let albedoColor = MaterialTextures[props.albedoTextureID].Sample(input.texCoord);
        let metallic = MaterialTextures[props.metalicTextureID].Sample(input.texCoord).x;
        let roughness = MaterialTextures[props.roughnessTextureID].Sample(input.texCoord).x;

        let tangentSpaceMat = float3x3(input.tangent, input.bitangent, input.normal);
        let normalVec = 2 * MaterialTextures[props.normalTextureID].Sample(input.texCoord).xyz - float3(1, 1, 1);

        FSOutput output;
        output.color = float4(albedoColor.xyz, roughness);
        output.normal = float4(normalize(mul(tangentSpaceMat, normalVec)), 1.0);
        output.position = float4(input.viewSpacePosition.xyz, metallic);

        return output;
    }
}

