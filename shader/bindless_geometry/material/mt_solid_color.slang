implementing material;

namespace material
{
    struct SolidColorProperties
    {
        float4 color;
        float normal;
        float metalic;
        float roughness;
    };

    [[vk::binding(3)]]
    uniform StructuredBuffer<SolidColorProperties> SolidColorPropertiesArray;

    FSOutput process_solid_color(FSInput input, const uint matInstanceID)
    {
        SolidColorProperties props = SolidColorPropertiesArray[matInstanceID];

        FSOutput output;
        output.color = props.color;
        output.normal = float4(input.normal, props.metalic);
        output.position = float4(input.viewSpacePosition.xyz, props.roughness);

        return output;
    }
}
