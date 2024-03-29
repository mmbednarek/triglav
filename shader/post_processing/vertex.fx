struct VSInput
{
};

struct VSOutput
{
	float4 Pos : SV_POSITION;

    [[vk::location(0)]]
    float2 TexCoord : TEXCOORD0;
};

static const float2 g_positions[4] =
{
    float2(-1.0, -1.0),
    float2(-1.0, 1.0),
    float2(1.0, -1.0),
    float2(1.0, 1.0),
};

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
    const float2 texPoint = g_positions[VertexIndex];

	VSOutput output = (VSOutput)0;
    output.TexCoord = (1 + texPoint.xy) * 0.5;
    output.Pos = float4(texPoint, 0.0, 1.0);

	return output;
}