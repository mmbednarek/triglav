module material;

__include material_core;
__include mt_solid_color;
__include mt_albedo_texture;
__include mt_albedo_normal_texture;
__include mt_all_texture;
__include texture_bindings;

namespace material
{
    public FSOutput process_material(FSInput fsInput)
    {
        const uint templateID = fsInput.materialID & 0b111;
        const uint instanceID = fsInput.materialID >> 3;

        switch (templateID)
        {
        case 0: return process_solid_color(fsInput, instanceID);
        case 1: return process_albedo_texture(fsInput, instanceID);
        case 2: return process_albedo_normal_texture(fsInput, instanceID);
        case 3: return process_all_texture(fsInput, instanceID);
        default: break;
        }

        FSOutput output;
        output.color = float4(1, 0, 0, 1);
        output.position = float4(fsInput.position.xyz, 0);
        output.normal = float4(normalize(fsInput.normal), 1.0);
        return output;
    }
}
