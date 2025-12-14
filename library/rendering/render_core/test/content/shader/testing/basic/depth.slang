[[vk::binding(0, 0)]]
cbuffer UniformBuffers
{
    float4x4 transform[2];
};

struct VSInput
{
    [[vk::location(0)]]
    float3 position: POSITION0;
    uint instanceID: SV_InstanceID;
};

struct FSInput
{
    float4 position : SV_POSITION;
    float3 objPos: POSITION0;
};

FSInput vs_main(VSInput input)
{
    FSInput output;
    output.position = mul(transform[input.instanceID], float4(input.position, 1));
    output.objPos = input.position;
    return output;
}


float4 fs_main(FSInput input) : SV_TARGET0
{
    return float4(input.objPos, 1.0);
}
