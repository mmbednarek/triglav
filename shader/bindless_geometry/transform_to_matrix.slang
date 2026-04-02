import triglav.core;

[[vk::binding(0)]]
StructuredBuffer<triglav::Transform> SB_Transforms;

[[vk::binding(1)]]
RWStructuredBuffer<float4x4> SB_Matrices;

struct OffsetCount
{
    uint offset;
    uint count;
};

[[vk::binding(2)]]
ConstantBuffer<OffsetCount> CB_TransformOffsetCount;

[shader("compute")]
[numthreads(256, 1, 1)]
void cs_main(uint thread_id : SV_DispatchThreadID)
{
    if (thread_id >= CB_TransformOffsetCount.count)
        return;

    const uint index = CB_TransformOffsetCount.offset + thread_id;
    triglav::Transform src_transform = SB_Transforms[index];
    if (src_transform.scale.x == 0.0)
        return;

    SB_Matrices[index] = SB_Transforms[index].matrix();
}
