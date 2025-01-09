import material.material;
import mesh.mesh;

/*

BINDING LAYOUT

0 - View Properties (Vertex)
1 - Scene Meshes (Vertex)
2 - Material Textures (Frag)
3 - MT_SolidColor Props
4 - MT_AlbedoTexture Props

*/

struct UniformViewProperties
{
    float4x4 view;
    float4x4 proj;
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

[shader("vertex")]
material::VSOutput vs_main(material::VSInput vsInput)
{
    material::VSOutput output;

    mesh::SceneMesh sceneMesh = SceneMeshes[get_draw_index()];

    float4 viewSpace = mul(UboViewProps.view, mul(sceneMesh.transform, float4(vsInput.position, 1.0)));
    float3x3 viewNormalMat = mul(float3x3(UboViewProps.view), float3x3(sceneMesh.normalTransform));

    output.texCoord = vsInput.texCoord;
    output.normal = mul(viewNormalMat, vsInput.normal);
    output.tangent = mul(viewNormalMat, vsInput.tangent);
    output.bitangent = mul(viewNormalMat, vsInput.bitangent);
    output.viewSpacePosition = viewSpace.xyz;
    output.position = mul(UboViewProps.proj, viewSpace);
    output.materialID = sceneMesh.materialID;

    return output;
}

[shader("fragment")]
material::FSOutput fs_main(material::FSInput fsInput)
{
    return material::process_material(fsInput);
}
