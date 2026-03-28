static const uint MAX_MATRICES = 6;

struct Task {
    uint destination_id;
    uint matrix_count;
    uint matrix_ids[MAX_MATRICES];
};

[[vk::binding(0)]]
StructuredBuffer<Task> SB_Tasks;

[[vk::binding(1)]]
ConstantBuffer<uint> CB_TaskCount;

[[vk::binding(2)]]
RWStructuredBuffer<float4x4> SB_Matrices;

[shader("compute")]
[numthreads(256, 1, 1)]
void cs_main(uint thread_id : SV_DispatchThreadID)
{
    if (thread_id >= CB_TaskCount)
        return;

    const Task task = SB_Tasks[thread_id];

    float4x4 result = SB_Matrices[task.matrix_ids[0]];
    for (int i = 1; i < task.matrix_count; ++i) {
        result = mul(result, SB_Matrices[task.matrix_ids[i]]);
    }

    SB_Matrices[task.destination_id] = result;
}
