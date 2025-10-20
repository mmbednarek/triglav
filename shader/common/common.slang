module common;

__include constants;
__include brdf;
__include shadow_map;
__include random;
__include view_properties;

namespace common
{

public uint get_draw_index() {
    return spirv_asm {
        OpCapability DrawParameters;
        result:$$uint = OpLoad builtin(DrawIndex:uint);
    };
}

public float4 to_linear_color(const float4 color)
{
    return float4(pow(color.xyz, GAMMA), color.w);
}

public float4 correct_gamma(const float4 color)
{
    return float4(pow(color.xyz, 1.0 / GAMMA), color.w);
}

}