implementing material;

namespace material
{
    struct AlbedoTextureProperties
    {
        uint albedoTextureID;
        float normal;
        float metalic;
        float roughness;
    };

    [[vk::binding(4)]]
    uniform StructuredBuffer<AlbedoTextureProperties> AlbedoTexturePropertiesArray;

    FSOutput process_albedo_texture(FSInput input, const uint matInstanceID)
    {
        AlbedoTextureProperties props = AlbedoTexturePropertiesArray[matInstanceID];

        float4 albedoColor = MaterialTextures[props.albedoTextureID].Sample(input.texCoord);

        FSOutput output;
        output.color = albedoColor;
        output.normal = float4(input.normal, props.metalic);
        output.position = float4(input.viewSpacePosition.xyz, props.roughness);

        return output;
    }
}
