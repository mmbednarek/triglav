import mesh.mesh;

struct UniformViewProperties
{
    float4x4 view;
    float4x4 proj;
    float4x4 invView;
    float4x4 invProj;
    float4x4 orient;
    float nearPlane;
    float farPlane;
};

[[vk::binding(0)]]
uniform ConstantBuffer<UniformViewProperties> UboViewProps;

[[vk::binding(1)]]
uniform StructuredBuffer<mesh::SceneMesh> SceneMeshes;

uint get_draw_index() {
    return spirv_asm {
        OpCapability DrawParameters;
        result:$$uint = OpLoad builtin(DrawIndex:uint);
    };
}

struct VSInput
{
    [[vk::location(0)]]
    float3 position : POSITION0;
};


[shader("vertex")]
float4 vs_main(VSInput vsInput) : SV_Position
{
    mesh::SceneMesh sceneMesh = SceneMeshes[get_draw_index()];
    return mul(UboViewProps.proj, mul(UboViewProps.view, mul(sceneMesh.transform, float4(vsInput.position, 1.0))));
}

[shader("fragment")]
void fs_main()
{
}