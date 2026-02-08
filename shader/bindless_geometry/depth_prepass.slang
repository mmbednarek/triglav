import triglav.core;
import triglav.mesh;

[[vk::binding(0)]]
uniform ConstantBuffer<triglav::ViewProperties> UboViewProps;

[[vk::binding(1)]]
uniform StructuredBuffer<triglav::mesh::DrawCall> SceneMeshes;

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION0;
};


[shader("vertex")]
float4 vs_main(VSInput vsInput) : SV_Position
{
    triglav::mesh::DrawCall sceneMesh = SceneMeshes[triglav::get_draw_index()];
    return mul(UboViewProps.proj, mul(UboViewProps.view, mul(sceneMesh.transform, float4(vsInput.position, 1.0))));
}

[shader("fragment")]
void fs_main()
{
}