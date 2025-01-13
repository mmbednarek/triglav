import mesh.mesh;

struct ShadowMapProperties
{
    float4x4 view_proj;
};

[[vk::binding(0)]]
uniform ConstantBuffer<ShadowMapProperties> shadowMapProps;

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
    return mul(shadowMapProps.view_proj, mul(sceneMesh.transform, float4(vsInput.position, 1.0)));
}

[shader("fragment")]
void fs_main()
{
}
