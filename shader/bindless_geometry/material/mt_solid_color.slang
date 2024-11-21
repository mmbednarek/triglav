implementing material;

namespace material
{
    struct SolidColorProperties
    {
        float3 color;
        float metalic;
        float roughness;
    };

    [[vk::binding(2)]]
    uniform StructuredBuffer<SolidColorProperties> SolidColorPropertiesArray;

    FSOutput process_solid_color(FSInput input, const uint matInstanceID)
    {
        SolidColorProperties props = SolidColorPropertiesArray[matInstanceID];

        FSOutput output;
        output.color = float4(props.color, 1);
        output.normal = float4(input.normal, props.metalic);
        output.position = float4(input.viewSpacePosition.xyz, props.roughness);

        return output;
    }
}