import triglav.core;

[[vk::binding(1)]]
uniform ConstantBuffer<triglav::ViewProperties> cb_view;

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
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};


float3 calculate_normal(float2 uv, float2 uv_to_world)
{
    uint3 dims;
    s_height_map.GetDimensions(0, dims.x, dims.y, dims.z);

    float dx = 0.5 / float(dims.x);
    float dy = 0.5 / float(dims.y);

    float hl = s_height_map.SampleLevel(uv + float2(-dx, 0), 0);
    float hr = s_height_map.SampleLevel(uv + float2( dx, 0), 0);
    float hb = s_height_map.SampleLevel(uv + float2(0, -dy), 0);
    float hf = s_height_map.SampleLevel(uv + float2(0,  dy), 0);

    float3 tangent = normalize(float3(dx * 240.0, 0.0, (hr - hl) * 30.0f));
    float3 bitangent = normalize(float3(0.0, dy * 240.0, (hf - hb) * 30.0f));

    return normalize(cross(tangent, bitangent));
}

[shader("domain")]
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
    v.uv = lerp(uv1, uv2, uv.y);

    // 2. Displacement: Sample height and move the vertex vertically
    float height = s_height_map.SampleLevel(v.uv, 0);
    pos.z += height * 30.0f; // Scale height by your terrain max height

    const float3 normal = calculate_normal(v.uv, float2(200.0, 200.0));

    // 3. Project to screen
    v.position = mul(cb_view.proj, mul(cb_view.view, float4(pos, 1.0)));
    v.normal = normal;
    return v;
}