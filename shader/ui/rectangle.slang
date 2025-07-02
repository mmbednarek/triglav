struct VSOutput
{
    float4 position: SV_Position;

    [[vk::location(0)]]
    float2 screenPos: POSITION0;
};

[[vk::binding(0)]]
cbuffer VertUBO
{
    float2 g_position;
    float2 g_viewportSize;
};

VSOutput vs_main([[vk::location(0)]] float2 inPosition: POSITION0)
{
    VSOutput out;
    out.position = float4(2 * (inPosition / g_viewportSize) - float2(1, 1), 0, 1);
    out.screenPos = inPosition - g_position;
    return out;
}

[[vk::binding(1)]]
cbuffer FragUBO
{
    float4 g_borderRadius;
    float4 g_borderColor;
    float4 g_backgroundColor;
    float2 g_rectSize;
    float g_borderWidth;
};

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

float4 sample_rect(float2 screenPos)
{
    if (screenPos.x > g_borderWidth && screenPos.y > g_borderWidth && screenPos.x <= (g_rectSize.x - g_borderWidth) && screenPos.y <= (g_rectSize.y - g_borderWidth)) {
        const bool isInside = is_inside_rect(g_borderRadius - float4(g_borderWidth, g_borderWidth, g_borderWidth, g_borderWidth), g_rectSize - 2*float2(g_borderWidth, g_borderWidth), screenPos - float2(g_borderWidth, g_borderWidth));
        if (isInside) {
            return g_backgroundColor;
        }
    }

    const bool isInsideBorder = is_inside_rect(g_borderRadius, g_rectSize, screenPos);
    return isInsideBorder ? g_borderColor : float4(0, 0, 0, 0);
}

float4 fs_main([[vk::location(0)]] float2 screenPos : POSITION0) : SV_Target0
{
    float4 values[4];

    values[0] = sample_rect(screenPos + float2(+0.25, +0.25));
    values[1] = sample_rect(screenPos + float2(+0.25, -0.25));
    values[2] = sample_rect(screenPos + float2(-0.25, +0.25));
    values[3] = sample_rect(screenPos + float2(-0.25, -0.25));

    float4 result = float4(0, 0, 0, 0);
    [unroll]
    for (int i = 0; i < 4; ++i) {
        result += values[i];
    }

    return 0.25 * result;
}
