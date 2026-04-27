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

// MT_USE_NORMAL_MAP
// MT_USE_ROUGHNESS_MAP
// MT_USE_METALIC_MAP

typealias VSInput = triglav::material::VSInput;
typealias VSOutput = triglav::material::VSOutput;

[[vk::binding(0)]]
uniform ConstantBuffer<triglav::ViewProperties> UboViewProps;

[[vk::binding(1)]]
uniform StructuredBuffer<triglav::mesh::DrawCall> DrawCalls;

#if MT_USE_BONES
[[vk::binding(4)]]
uniform StructuredBuffer<float4x4> sb_matrices;
#endif

float3 fix_normal(float3 v) {
    return float3(v.x, v.y, v.z);
}

[shader("vertex")]
VSOutput vs_main(VSInput vertex_input)
{
    VSOutput output;
    triglav::mesh::DrawCall drawCall = DrawCalls[triglav::get_draw_index()];

#if MT_USE_BONES
    const float4 vertex_position = float4(vertex_input.position, 1);
    float4 position = float4(0, 0, 0, 0);
    for (int i = 0; i < 4; ++i) {
        const int bone_id = vertex_input.bone_ids[i];
        if (bone_id == -1)
            continue;

        const float4 local_position = mul(sb_matrices[drawCall.matrixOffset + bone_id], vertex_position);
        position += local_position * vertex_input.bone_weights[i];
    }
#else
    const float4 position = float4(vertex_input.position, 1);
#endif

    float4 pos = mul(drawCall.transform, position);
    float4 viewSpace = mul(UboViewProps.view, pos);
    // float3x3 viewNormalMat = mul(float3x3(UboViewProps.view), float3x3(drawCall.normalTransform));
    float3x3 normalTrans = float3x3(drawCall.normalTransform);

    output.texCoord = vertex_input.texCoord;
    output.normal = mul(normalTrans, vertex_input.normal);
#if MT_USE_NORMAL_MAP
    output.tangent = mul(normalTrans, vertex_input.tangent.xyz);
    output.bitangent = mul(normalTrans, vertex_input.tangent.w * cross(vertex_input.normal, vertex_input.tangent.xyz));
#endif
    output.viewSpacePosition = pos.xyz;
    output.position = mul(UboViewProps.proj, viewSpace);
    output.materialID = drawCall.materialID;

    return output;
}
