float4 vs_main(float4 position: POSITION0) : SV_POSITION
{
    return position;
}

[[vk::binding(0, 0)]]
cbuffer UniformBuffer : register(b0)
{
    float4 color;
};

float4 fs_main() : SV_TARGET0
{
    return color;
}
