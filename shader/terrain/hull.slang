import triglav.core;

struct Vertex
{
    float3 position  : POSITION;
    float2 tex_coord : TEXCOORD0;
};

[[vk::binding(0)]]
uniform ConstantBuffer<triglav::ViewProperties> cb_view;

static const float min_distance = 1.0;
static const float max_distance = 20.0;
static const float max_tess_level = 20.0;

// struct TessParams
// {
//     float min_distance;
//     float max_distance;
//     float max_tess_level;
// };
//
// [[vk::binding(1)]]
// uniform ConstantBuffer<TessParams> cb_tess_params;

struct PatchConstants
{
    float edges[4]  : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};


float distance_to_tes_level(float dis) {
    return clamp(max_tess_level * (1.0 - (dis - min_distance) / (max_distance - min_distance)), 1.0, max_tess_level);
}

PatchConstants patch_constant(InputPatch<Vertex, 4> patch)
{
    PatchConstants pt;

    // Calculate distance from camera to center of the patch
    const float3 center = (patch[0].position + patch[1].position + patch[2].position + patch[3].position) * 0.25f;
    const float distance_center = distance(cb_view.viewPos.xyz, center);

    pt.inside[0] = pt.inside[1] = distance_to_tes_level(distance_center);

    const float3 edge_a = (patch[0].position + patch[1].position) * 0.5f;
    const float3 edge_b = (patch[1].position + patch[2].position) * 0.5f;
    const float3 edge_c = (patch[2].position + patch[3].position) * 0.5f;
    const float3 edge_d = (patch[3].position + patch[0].position) * 0.5f;

    const float distance_a = distance(cb_view.viewPos.xyz, edge_a);
    const float distance_b = distance(cb_view.viewPos.xyz, edge_b);
    const float distance_c = distance(cb_view.viewPos.xyz, edge_c);
    const float distance_d = distance(cb_view.viewPos.xyz, edge_d);

    // Calculate tessellation factor: closer = higher detail

    pt.edges[1] = distance_to_tes_level(distance_a);
    pt.edges[2] = distance_to_tes_level(distance_b);
    pt.edges[3] = distance_to_tes_level(distance_c);
    pt.edges[0] = distance_to_tes_level(distance_d);

    return pt;
}

[shader("hull")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("patch_constant")]
Vertex hs_main(InputPatch<Vertex, 4> patch, uint id : SV_OutputControlPointID)
{
    return patch[id]; // Pass-through
}

