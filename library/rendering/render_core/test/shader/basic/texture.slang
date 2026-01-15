struct VSInput
{
    float2 position: POSITION0;
    float2 texCoord: TEXCOORD0;
};

struct VSOutput
{
    float4 position: SV_POSITION;
    float2 texCoord: TEXCOORD0;
};

VSOutput vs_main(VSInput input)
{
    VSOutput out;
    out.position = float4(input.position, 0, 1);
    out.texCoord = input.texCoord;
    return out;
}

typealias FSInput = VSOutput;

[[vk::binding(0, 0)]]
uniform Sampler2D<float4> InTexture;

float4 fs_main(FSInput input) : SV_TARGET0
{
    return InTexture.Sample(input.texCoord);
}
