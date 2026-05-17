import triglav.core;

struct Vertex
{
    float3 position  : POSITION;
    float2 tex_coord : TEXCOORD0;
};

[[vk::binding(0)]]
uniform ConstantBuffer<triglav::ViewProperties> cb_view;

static const float min_distance = 5.0;
static const float max_distance = 200.0;
static const float max_tess_level = 80.0;

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

PatchConstants patch_constant(InputPatch<Vertex, 4> patch)
{
    PatchConstants pt;

    // Calculate distance from camera to center of the patch
    float3 center = (patch[0].position + patch[1].position + patch[2].position + patch[3].position) * 0.25f;
    float d = distance(cb_view.viewPos.xyz, center);

    // Calculate tessellation factor: closer = higher detail
    float factor = clamp(max_tess_level * (1.0 - (d - min_distance) / (max_distance - min_distance)), 1.0, max_tess_level);

    pt.edges[0] = pt.edges[1] = pt.edges[2] = pt.edges[3] = factor;
    pt.inside[0] = pt.inside[1] = factor;

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

