import triglav.core;
import triglav.mesh;

typealias DrawCall = triglav::mesh::DrawCall;

struct ShadowMapProperties
{
    float4x4 view_proj;
};

[[vk::binding(0)]]
uniform ConstantBuffer<ShadowMapProperties> shadowMapProps;

[[vk::binding(1)]]
uniform StructuredBuffer<DrawCall> DrawCalls;

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION0;
};

[shader("vertex")]
float4 vs_main(VSInput vsInput) : SV_Position
{
    DrawCall drawCall = DrawCalls[triglav::get_draw_index()];
    return mul(shadowMapProps.view_proj, mul(drawCall.transform, float4(vsInput.position, 1.0)));
}

[shader("fragment")]
void fs_main()
{
}
