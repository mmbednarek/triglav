import triglav.core;
import triglav.ui;

[[vk::binding(0)]]
cbuffer VertUBO
{
    float2 g_viewportSize;
};

[[vk::binding(1)]]
StructuredBuffer<triglav::ui::RectDrawCall> g_drawCallBuffer;

struct VSOutput
{
    float4 position: SV_Position;

    [[vk::location(0)]]
    float2 screenPos;

    [[vk::location(1)]]
    nointerpolation float2 screenOffset;

    [[vk::location(2)]]
    nointerpolation float2 rectSize;

    [[vk::location(3)]]
    nointerpolation float4 borderRadius;

    [[vk::location(4)]]
    nointerpolation float4 borderColor;

    [[vk::location(5)]]
    nointerpolation float4 backgroundColor;

    [[vk::location(6)]]
    nointerpolation float4 croppingMask;

    [[vk::location(7)]]
    nointerpolation float borderWidth;
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
    const triglav::ui::RectPrimitive prim = g_drawCallBuffer[triglav::get_draw_index()].primitive;

    const float2 vertex = g_vertexIndex[vid];
    const float2 posOnRect = round(vertex * prim.dimensions.zw);
    const float2 ssPosition = round(prim.dimensions.xy) + posOnRect;

    VSOutput out;
    out.position = float4(2 * (ssPosition / g_viewportSize) - float2(1, 1), 0, 1);
    out.screenPos = posOnRect;
    out.screenOffset = prim.dimensions.xy;
    out.rectSize = prim.dimensions.zw;
    out.borderRadius = prim.borderRadius;
    out.borderColor = triglav::to_linear_color(prim.borderColor);
    out.backgroundColor = triglav::to_linear_color(prim.backgroundColor);
    out.croppingMask = prim.croppingMask;
    out.borderWidth = prim.borderWidth;
    return out;
}

bool is_inside_radius(float2 position, float2 center, float radius)
{
    return distance(position, center) < radius;
}

bool is_inside_rect(float4 borderRadius, float2 rectSize, float2 position)
{
    const float2 topLeft = float2(borderRadius.x, borderRadius.x);
    if (position.x < topLeft.x && position.y < topLeft.y) {
        return is_inside_radius(position, topLeft, borderRadius.x);
    }

    const float2 topRight = float2(rectSize.x - borderRadius.y, borderRadius.y);
    if (position.x >= topRight.x && position.y < topRight.y) {
        return is_inside_radius(position, topRight, borderRadius.y);
    }

    const float2 bottomLeft = float2(borderRadius.z, rectSize.y - borderRadius.z);
    if (position.x < bottomLeft.x && position.y >= bottomLeft.y) {
        return is_inside_radius(position, bottomLeft, borderRadius.z);
    }

    const float2 bottomRight = float2(rectSize.x - borderRadius.w, rectSize.y - borderRadius.w);
    if (position.x >= bottomRight.x && position.y >= bottomRight.y) {
        return is_inside_radius(position, bottomRight, borderRadius.w);
    }

    return true;
}

float4 sample_rect(in FSInput input, float2 screenPos)
{
    if (screenPos.x > input.borderWidth && screenPos.y > input.borderWidth && screenPos.x <= (input.rectSize.x - input.borderWidth) && screenPos.y <= (input.rectSize.y - input.borderWidth)) {
        const bool isInside = is_inside_rect(input.borderRadius - float4(input.borderWidth, input.borderWidth, input.borderWidth, input.borderWidth), input.rectSize - 2*float2(input.borderWidth, input.borderWidth), screenPos - float2(input.borderWidth, input.borderWidth));
        if (isInside) {
            return input.backgroundColor;
        }
    }

    const bool isInsideBorder = is_inside_rect(input.borderRadius, input.rectSize, screenPos);
    return isInsideBorder ? input.borderColor : float4(0, 0, 0, 0);
}

float4 fs_main(FSInput input) : SV_Target0
{
    float2 xy = input.screenOffset + input.screenPos;
    if ((xy.x < input.croppingMask.x) ||
        (xy.x > (input.croppingMask.x + input.croppingMask.z)) ||
        (xy.y < input.croppingMask.y) ||
        (xy.y > (input.croppingMask.y + input.croppingMask.w))) {
        return float4(0, 0, 0, 0);
    }

    float4 values[4];
    values[0] = sample_rect(input, input.screenPos + float2(+0.25, +0.25));
    values[1] = sample_rect(input, input.screenPos + float2(+0.25, -0.25));
    values[2] = sample_rect(input, input.screenPos + float2(-0.25, +0.25));
    values[3] = sample_rect(input, input.screenPos + float2(-0.25, -0.25));

    float3 result = float3(0, 0, 0);
    float divisor = 0.0;
    [unroll]
    for (int i = 0; i < 4; ++i) {
        float4 val = values[i];
        result += val.xyz * val.a;
        divisor += val.a;
    }
    return float4(result / divisor, 0.25 * divisor);
}
