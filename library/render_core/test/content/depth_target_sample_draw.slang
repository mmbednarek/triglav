[[vk::binding(0, 0)]]
cbuffer UniformBuffers
{
    float4x4 transform;
};

struct VSInput
{
    [[vk::location(0)]]
    float3 position: POSITION0;
};

float4 vs_main(VSInput input) : SV_Position
{
    return mul(transform, float4(input.position, 1));
}

void fs_main()
{
}
