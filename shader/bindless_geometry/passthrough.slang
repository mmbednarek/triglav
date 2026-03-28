import triglav.mesh;

[[vk::binding(0)]]
StructuredBuffer<triglav::mesh::SceneMesh> in_scene_meshes;

[[vk::binding(1)]]
ConstantBuffer<uint> in_mesh_count;

[[vk::binding(2)]]
StructuredBuffer<float4x4> SB_TransformMatrices;

[[vk::binding(3)]]
RWStructuredBuffer<triglav::mesh::DrawCall> out_draw_calls_base;

[[vk::binding(4)]]
RWStructuredBuffer<triglav::mesh::DrawCall> out_draw_calls_normal_map;

groupshared uint out_counts[2];

[numthreads(256, 1, 1)]
void cs_main(uint thread_id : SV_DispatchThreadID)
{
    if (thread_id >= in_mesh_count)
        return;

    if (thread_id == 0) {
        out_counts[0] = 0;
        out_counts[1] = 0;
        GroupMemoryBarrier();
    }

    var mesh = in_scene_meshes[thread_id];
    const uint template_id = mesh.materialID & 0b111;
    mesh.materialID >>= 3;

    if (template_id == 0) {
        uint dst_index = 0;
        InterlockedAdd(out_counts[0], 1, dst_index);
        out_draw_calls_base[dst_index] = mesh.to_draw_call(SB_TransformMatrices);
    } else {
        uint dst_index = 0;
        InterlockedAdd(out_counts[1], 1, dst_index);
        out_draw_calls_normal_map[dst_index] = mesh.to_draw_call(SB_TransformMatrices);
    }
}
