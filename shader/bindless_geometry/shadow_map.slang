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

#if MT_USE_BONES
[[vk::binding(2)]]
uniform StructuredBuffer<float4x4> sb_matrices;
#endif

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION0;
#if MT_USE_BONES
    [[vk::location(1)]]
    int4 bone_ids : BLENDINDICES0;
    [[vk::location(2)]]
    float4 bone_weights : BLENDWEIGHT0;
#endif
};

[shader("vertex")]
float4 vs_main(VSInput vsInput) : SV_Position
{
    DrawCall drawCall = DrawCalls[triglav::get_draw_index()];

#if MT_USE_BONES
    const float4 vertex_position = float4(vsInput.position, 1);
    float4 position = float4(0, 0, 0, 0);
    for (int i = 0; i < 4; ++i) {
        const int bone_id = vsInput.bone_ids[i];
        if (bone_id == -1)
            continue;

        const float4 local_position = mul(sb_matrices[drawCall.matrixOffset + bone_id], vertex_position);
        position += local_position * vsInput.bone_weights[i];
    }
#else
    const float4 position = float4(vsInput.position, 1);
#endif

    return mul(shadowMapProps.view_proj, mul(drawCall.transform, position));
}

[shader("fragment")]
void fs_main()
{
}
