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

float4 calculate_radius(float2 screenPos, float2 corner, float radius)
{
    float distance = distance(screenPos, corner);
    if (distance < radius) {
        const float alpha = 1.0 - pow(distance / radius, 20.0);
        const float alpha_rad = 1.0 - pow(distance / (radius - g_borderWidth), 10.0);
        if (distance > (radius - g_borderWidth - 0.6)) {
            return float4(g_borderColor.rgb, g_borderColor.a * alpha);
        } else {
            return float4(g_backgroundColor.rgb * alpha_rad + g_borderColor.rgb * (1 - alpha_rad), g_backgroundColor.a * alpha);
        }
    } else {
        return float4(0.0, 0.0, 0.0, 0.0);
    }
}

float4 fs_main([[vk::location(0)]] float2 screenPos : POSITION0) : SV_Target0
{
    const float2 topLeft = float2(g_borderRadius.x - 1, g_borderRadius.x - 1);
    const float2 topRight = float2(g_rectSize.x - g_borderRadius.y, g_borderRadius.y - 1);
    const float2 bottomLeft = float2(g_borderRadius.z - 1, g_rectSize.y - g_borderRadius.z + 1);
    const float2 bottomRight = float2(g_rectSize.x - g_borderRadius.w, g_rectSize.y - g_borderRadius.w + 1);

    if (screenPos.x < topLeft.x && screenPos.y < topLeft.y) {
        return calculate_radius(screenPos, topLeft, g_borderRadius.x);
    }

    if (screenPos.x > topRight.x && screenPos.y < topRight.y) {
        return calculate_radius(screenPos, topRight, g_borderRadius.y);
    }

    if (screenPos.x < bottomLeft.x && screenPos.y > bottomLeft.y) {
        return calculate_radius(screenPos, bottomLeft, g_borderRadius.z);
    }

    if (screenPos.x > bottomRight.x && screenPos.y > bottomRight.y) {
        return calculate_radius(screenPos, bottomRight, g_borderRadius.w);
    }

    if ((screenPos.x < (g_borderWidth-0.5)) ||
        (screenPos.y < (g_borderWidth-0.5)) ||
        (screenPos.x > (g_rectSize.x - g_borderWidth-0.5)) ||
        (screenPos.y > (g_rectSize.y - g_borderWidth))) {
        return g_borderColor;
    }


    return g_backgroundColor;
}
