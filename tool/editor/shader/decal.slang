import triglav.core;

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION0;
};

struct VSOutput
{
    float4 screen_coord: TEXCOORD0;
    float4 position : SV_POSITION;
};

[[vk::binding(0)]]
ConstantBuffer<triglav::ViewProperties> cb_view_properties;

[[vk::binding(1)]]
ConstantBuffer<float4x4> cb_matrix;

[shader("vertex")]
VSOutput vs_main(VSInput input)
{
    const float4 world_space_pos = mul(cb_matrix, float4(input.position, 1.0));
    const float4 view_space_pos = mul(cb_view_properties.view, world_space_pos);
    const float4 projected_pos = mul(cb_view_properties.proj, view_space_pos);

    const float2 coord = projected_pos.xy / projected_pos.w;

    VSOutput output;
    output.position = projected_pos;
    output.screen_coord = projected_pos;
    return output;
}

typealias FSInput = VSOutput;

struct FSOutput {
    float4 albedo: SV_Target0;
    float4 material: SV_Target1;
}

struct DecalInfo
{
    float3 center;
    float radius;
}

[[vk::binding(2)]]
uniform ConstantBuffer<DecalInfo> cb_info;

[[vk::binding(3)]]
uniform Texture2D<float> t_depth;

[shader("fragment")]
FSOutput fs_main(FSInput input) {
    float2 screen_coord = 0.5 * ((input.screen_coord.xy / input.screen_coord.w) + float2(1));
    float3 position = cb_view_properties.depth_world_pos(t_depth, screen_coord);

    float dist = distance(cb_info.center, position.xyz);
    if (dist >= cb_info.radius) {
        FSOutput output;
        output.albedo = float4(0);
        output.material = float4(0);
        return output;
    }
    if (dist >= cb_info.radius - 0.25) {
        FSOutput output;
        output.albedo = float4(1, 1, 1, 0.25);
        output.material = float4(0);
        return output;
    }

    FSOutput output;
    output.albedo = float4(1, 1, 1, 0.75 * (1.0 - dist / cb_info.radius));
    output.material = float4(1, 0, 0, 0);
    return output;
}
