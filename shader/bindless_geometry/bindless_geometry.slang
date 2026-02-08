import triglav.core;
import triglav.material;
import triglav.mesh;

/*

BINDING LAYOUT

0 - View Properties (Vertex)
1 - Scene Meshes (Vertex)
2 - Material Textures (Frag)
3 - MT_SolidColor Props
4 - MT_AlbedoTexture Props

*/

typealias VSInput = triglav::material::VSInput;
typealias VSOutput = triglav::material::VSOutput;

[[vk::binding(0)]]
uniform ConstantBuffer<triglav::ViewProperties> UboViewProps;

[[vk::binding(1)]]
uniform StructuredBuffer<triglav::mesh::DrawCall> DrawCalls;

[shader("vertex")]
VSOutput vs_main(VSInput vsInput)
{
    VSOutput output;

    triglav::mesh::DrawCall drawCall = DrawCalls[triglav::get_draw_index()];

    float4 viewSpace = mul(UboViewProps.view, mul(drawCall.transform, float4(vsInput.position, 1.0)));
    float3x3 viewNormalMat = mul(float3x3(UboViewProps.view), float3x3(drawCall.normalTransform));

    output.texCoord = vsInput.texCoord;
    output.normal = mul(viewNormalMat, vsInput.normal);
    output.tangent = mul(viewNormalMat, vsInput.tangent.xyz);
    output.bitangent = mul(viewNormalMat, vsInput.tangent.w * cross(vsInput.normal, vsInput.tangent.xyz));
    output.viewSpacePosition = viewSpace.xyz;
    output.position = mul(UboViewProps.proj, viewSpace);
    output.materialID = drawCall.materialID;

    return output;
}
