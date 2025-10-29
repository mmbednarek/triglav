import "../common/common.slang";

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    [[vk::location(0)]]
    float2 screen_coord : TEXCOORD0;
    [[vk::location(1)]]
    float depth;
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
    output.screen_coord = 0.5*(output.position.xy / output.position.w + float2(1, 1));
    output.depth = output.position.z / output.position.w;
//     output.screen_coord = float2(0, 0);
//     output.screen_coord = output.position.xy;
    return output;
}

typealias FSInput = VSOutput;

[[vk::binding(2)]]
Sampler2D<float4> g_back_texture;

[[vk::binding(3)]]
Sampler2D<float> g_depth;

[shader("fragment")]
float4 fs_main(FSInput input) : SV_Target0 {
    const float4 color = g_back_texture.Sample(float2(input.screen_coord.x, input.screen_coord.y));
//     const float4 inverted = common::to_linear_color(float4(float3(1) - common::correct_gamma(color).xyz, 1));

    const float depth = g_depth.Sample(input.screen_coord);
    if (depth > input.depth) {
        return float4(1, 0.5, 0, 1);
    } else {
        return lerp(float4(1, 0.5, 0, 1), color, 0.9);
    }
}
