import text_processing;

struct VSOutput
{
    float4 position: SV_Position;

    [[vk::location(0)]]
    float2 texCoord: TEXCOORD0;

    [[vk::location(1)]]
    float4 color: COLOR0;

    [[vk::location(2)]]
    nointerpolation uint atlasID;

    [[vk::location(3)]]
    float4 crop;
}

[[vk::binding(0)]]
uniform StructuredBuffer<text_processing::DrawCall> g_inDrawCalls;

uint get_draw_index() {
    return spirv_asm {
        OpCapability DrawParameters;
        result:$$uint = OpLoad builtin(DrawIndex:uint);
    };
}

[[vk::push_constant]]
cbuffer PushConstants
{
    int2 g_screenSize;
};

VSOutput vs_main(text_processing::Vertex inVertex)
{
    text_processing::DrawCall drawCall = g_inDrawCalls[get_draw_index()];

    const float2 screenSpacePos = 2 * ((inVertex.position + drawCall.position) / float2(g_screenSize)) - float2(1, 1);

    VSOutput out;
    out.position = float4(screenSpacePos, 0, 1);
    out.texCoord = inVertex.uv;
    out.color = drawCall.color;
    out.atlasID = drawCall.atlasID;
    out.crop = drawCall.crop;

    return out;
}

[[vk::binding(1)]]
uniform Sampler2D<float> g_atlases[];

float4 fs_main(VSOutput inFrag) : SV_Target0
{
    float2 xy = inFrag.position.xy;
    if (xy.x >= inFrag.crop.x && xy.x <= (inFrag.crop.x + inFrag.crop.z) && xy.y >= inFrag.crop.y && xy.y <= (inFrag.crop.y + inFrag.crop.w)) {
        return float4(inFrag.color.xyz, g_atlases[inFrag.atlasID].Sample(inFrag.texCoord));
    }
    return float4(0, 0, 0, 0);
}
