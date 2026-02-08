import triglav.mesh;

[[vk::binding(0)]]
StructuredBuffer<triglav::mesh::SceneMesh> in_scene_meshes;

[[vk::binding(1)]]
ConstantBuffer<uint> in_mesh_count;

[[vk::binding(2)]]
RWStructuredBuffer<triglav::mesh::DrawCall> out_draw_calls;

[numthreads(256, 1, 1)]
void cs_main(uint thread_id : SV_DispatchThreadID)
{
    if (thread_id >= in_mesh_count)
        return;

    out_draw_calls[thread_id] = in_scene_meshes[thread_id].to_draw_call();
}
