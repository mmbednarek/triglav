import triglav.core;

[[vk::binding(1)]]
uniform ConstantBuffer<float4x4> cb_shadow_prop;

[[vk::binding(2)]]
Sampler2D<float> s_height_map;

struct VertexIn
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VertexOut
{
    float4 position : SV_POSITION;
};

[shader("domain")]
[partitioning("fractional_even")]
[domain("quad")]
VertexOut ds_main(float2 uv : SV_DomainLocation, const OutputPatch<VertexIn, 4> patch)
{
    VertexOut v;

    // 1. Bilinear interpolation of the patch corners to find the flat position
    float3 p1 = lerp(patch[0].position, patch[1].position, uv.x);
    float3 p2 = lerp(patch[3].position, patch[2].position, uv.x);
    float3 pos = lerp(p1, p2, uv.y);

    float2 uv1 = lerp(patch[0].uv, patch[1].uv, uv.x);
    float2 uv2 = lerp(patch[3].uv, patch[2].uv, uv.x);
    float2 uv = lerp(uv1, uv2, uv.y);

    // 2. Displacement: Sample height and move the vertex vertically
    float height = s_height_map.SampleLevel(uv, 0);
    pos.z += height * 30.0f; // Scale height by your terrain max height

    // 3. Project to screen
    v.position = mul(cb_shadow_prop, float4(pos, 1.0));
    return v;
}
