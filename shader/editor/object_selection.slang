import "../common/common.slang";

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

[[vk::binding(0)]]
ConstantBuffer<common::ViewProperties> g_view_properties;

[[vk::binding(1)]]
cbuffer EditorOverlayUB {
   float4x4 g_object_transform;
};

[shader("vertex")]
VSOutput vs_main(VSInput input)
{
    const float4 world_space_pos = mul(g_object_transform, float4(input.position, 1.0));
    const float4 view_space_pos = mul(g_view_properties.view, world_space_pos);

    VSOutput output;
    output.position = mul(g_view_properties.proj, view_space_pos);
    return output;
}

[shader("fragment")]
float4 fs_main() : SV_Target0 {
    return float4(0.5, 0.5, 0.5, 1.0);
}
