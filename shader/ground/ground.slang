struct VSOutput
{
    float4 position : SV_POSITION;

    [[vk::location(0)]]
    float4 worldPos : POSITION0;
    [[vk::location(1)]]
    float3 normal : NORMAL;
    [[vk::location(2)]]
    float4 viewPos : POSITION1;
};

struct GroundUBO
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0)]]
cbuffer ubo
{
    GroundUBO ubo;
};

static float4 g_positions[4] =
{
    float4(-1.0, 1.0, 0.0, 1.0),
    float4(-1.0, -1.0, 0.0, 1.0),
    float4(1.0, 1.0, 0.0, 1.0),
    float4(1.0, -1.0, 0.0, 1.0),
};

[shader("vertex")]
VSOutput vs_main(uint vertexIndex : SV_VertexID)
{
    float4 position = g_positions[vertexIndex];

    VSOutput output;

    output.worldPos = mul(ubo.model, position);
    output.viewPos = mul(ubo.view, output.worldPos);
    output.normal = mul(float3x3(ubo.view), float3(0, 0, -1));
    output.position  = mul(ubo.proj, output.viewPos);

    return output;
}

struct FSOutput
{
    [[vk::location(0)]]
    float4 color : SV_TARGET0;
    [[vk::location(1)]]
    float4 position : SV_TARGET1;
    [[vk::location(2)]]
    float4 normal : SV_TARGET2;
};

[[vk::binding(1)]]
Sampler2D<float> texTile;

[shader("fragment")]
FSOutput fs_main(VSOutput input) {
    float3 pos = input.viewPos.xyz / input.viewPos.w;
    float dist = clamp(2.0 / sqrt(abs(pos.z)) - 0.1, 0.0, 1.0);
    float color = lerp(0.7, texTile.Sample(0.2 * input.worldPos.xy), dist);

    FSOutput output;
    output.color = float4(float3(color), 1.0 - 0.4*color);
    output.position = float4(input.viewPos.xyz, 0.0);
    output.normal = float4(normalize(input.normal), 1.0);

    return output;
}
