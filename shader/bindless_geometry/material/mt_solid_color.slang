implementing material;

namespace material
{
    struct SolidColorProperties
    {
        float4 color;
        float normal;
        float roughness;
        float metalic;
    };

    [[vk::binding(3)]]
    uniform StructuredBuffer<SolidColorProperties> SolidColorPropertiesArray;

    FSOutput process_solid_color(FSInput input, const uint matInstanceID)
    {
        SolidColorProperties props = SolidColorPropertiesArray[matInstanceID];

        FSOutput output;
        output.color = float4(props.color.xyz, props.roughness);
        output.normal = float4(input.normal, 1.0);
        output.position = float4(input.viewSpacePosition.xyz, props.metalic);

        return output;
    }
}
