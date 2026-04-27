import triglav.mesh;

[[vk::binding(0)]]
StructuredBuffer<triglav::mesh::SceneMesh> in_scene_meshes;

[[vk::binding(1)]]
ConstantBuffer<uint> in_mesh_count;

[[vk::binding(2)]]
StructuredBuffer<float4x4> SB_TransformMatrices;

[[vk::binding(3)]]
RWStructuredBuffer<uint32_t> SB_CountBuffer;

[[vk::binding(4)]]
RWStructuredBuffer<triglav::mesh::DrawCall> out_draw_calls_base;

[[vk::binding(5)]]
RWStructuredBuffer<triglav::mesh::DrawCall> out_draw_calls_normal_map;

[[vk::binding(6)]]
RWStructuredBuffer<triglav::mesh::DrawCall> out_draw_calls_skeletal;

[[vk::binding(7)]]
RWStructuredBuffer<triglav::mesh::DrawCall> out_draw_calls_skeletal_normal_map;

[numthreads(256, 1, 1)]
void cs_main(uint thread_id : SV_DispatchThreadID)
{
    if (thread_id >= in_mesh_count)
        return;

    if (thread_id == 0) {
        for (int i = 0; i < 4; ++i) {
            SB_CountBuffer[i] = 0;
        }
        GroupMemoryBarrier();
    }

    var mesh = in_scene_meshes[thread_id];
    const uint template_id = mesh.materialID & 0b111;
    const uint vertex_layout_id = (mesh.is_skeletal() ? 2 : 0) + (template_id == 0 ? 0 : 1);

    mesh.materialID >>= 3;

    uint dst_index = 0;
    InterlockedAdd(SB_CountBuffer[vertex_layout_id], 1, dst_index);

    triglav::mesh::DrawCall draw_call = mesh.to_draw_call(SB_TransformMatrices);

    switch (vertex_layout_id) {
    case 0:
        out_draw_calls_base[dst_index] = draw_call;
        break;
    case 1:
        out_draw_calls_normal_map[dst_index] = draw_call;
        break;
    case 2:
        out_draw_calls_skeletal[dst_index] = draw_call;
        break;
    case 3:
        out_draw_calls_skeletal_normal_map[dst_index] = draw_call;
        break;
    }
}
