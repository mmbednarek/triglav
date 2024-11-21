import mesh;

[[vk::binding(0)]]
StructuredBuffer<mesh::SceneMesh> SceneMeshes;

[[vk::binding(1)]]
RWStructuredBuffer<mesh::SceneMesh> DrawCalls;

[numthreads(1028, 1, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID)
{
    mesh::SceneMesh sceneMesh = SceneMeshes[threadID.x];

    // TODO: Implement frustum and occlusion culling
}
