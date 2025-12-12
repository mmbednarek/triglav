import "../common/common.slang";
import sprite_api;

[[vk::binding(0)]]
cbuffer VertUBO
{
    float2 g_viewportSize;
};

[[vk::binding(1)]]
StructuredBuffer<sprite_api::DrawCall> g_drawCallBuffer;

struct VSOutput
{
    float4 position: SV_Position;

    [[vk::location(0)]]
    float2 texCoord;

    [[vk::location(1)]]
    nointerpolation uint32_t imageID;

    [[vk::location(2)]]
    float2 screenPosition;

    [[vk::location(3)]]
    nointerpolation float4 croppingMask;
};

typealias FSInput = VSOutput;

static float2 g_vertexIndex[] = {
    float2(0, 0),
    float2(0, 1),
    float2(1, 0),
    float2(1, 0),
    float2(0, 1),
    float2(1, 1),
};

VSOutput vs_main(uint vid: SV_VertexID)
{
    const sprite_api::Primitive prim = g_drawCallBuffer[common::get_draw_index()].primitive;

    float2 vertex = g_vertexIndex[vid];
    float2 ssPosition = round(prim.spriteRect.xy) + vertex*round(prim.spriteRect.zw);

    VSOutput out;
    out.position = float4(2 * (ssPosition / g_viewportSize) - float2(1, 1), 0, 1);
    out.texCoord = prim.uvRect.xy + vertex*prim.uvRect.zw;
    out.imageID = prim.imageID;
    out.screenPosition = ssPosition;
    out.croppingMask = prim.croppingMask;
    return out;
}

[[vk::binding(2)]]
uniform Sampler2D<float4> g_textures[];

float4 fs_main(FSInput input) : SV_Target0
{
    float2 xy = input.screenPosition;
    if ((xy.x < input.croppingMask.x) ||
        (xy.x > (input.croppingMask.x + input.croppingMask.z)) ||
        (xy.y < input.croppingMask.y) ||
        (xy.y > (input.croppingMask.y + input.croppingMask.w))) {
        return float4(0, 0, 0, 0);
    }

    return g_textures[input.imageID].Sample(input.texCoord);
}
