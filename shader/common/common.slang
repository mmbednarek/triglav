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

}