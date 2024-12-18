implementing material;

namespace material
{
    struct AlbedoTextureProperties
    {
        uint albedoTextureID;
        float normal;
        float roughness;
        float metalic;
    };

    [[vk::binding(4)]]
    uniform StructuredBuffer<AlbedoTextureProperties> AlbedoTexturePropertiesArray;

    FSOutput process_albedo_texture(FSInput input, const uint matInstanceID)
    {
        AlbedoTextureProperties props = AlbedoTexturePropertiesArray[matInstanceID];

        float4 albedoColor = MaterialTextures[props.albedoTextureID].Sample(input.texCoord);

        FSOutput output;
        output.color = float4(albedoColor.xyz, props.roughness);
        output.normal = float4(normalize(input.normal), 1.0);
        output.position = float4(input.viewSpacePosition.xyz, props.metalic);

        return output;
    }
}
