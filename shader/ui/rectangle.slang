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
    float4 g_backgroundColor;
    float2 g_rectSize;
};

float4 fs_main([[vk::location(0)]] float2 screenPos : POSITION0) : SV_Target0
{
    const float2 topLeft = float2(g_borderRadius.x, g_borderRadius.x);
    const float2 topRight = float2(g_rectSize.x - g_borderRadius.y, g_borderRadius.y);
    const float2 bottomLeft = float2(g_borderRadius.z, g_rectSize.y - g_borderRadius.z);
    const float2 bottomRight = float2(g_rectSize.x - g_borderRadius.w, g_rectSize.y - g_borderRadius.w);

    if (screenPos.x < topLeft.x && screenPos.y < topLeft.y) {
        float distance = distance(screenPos, topLeft);
        if (distance < g_borderRadius.x) {
            float alpha = 1.0 - pow(distance / g_borderRadius.x, 50.0);
            return float4(g_backgroundColor.rgb, g_backgroundColor.a * alpha);
        } else {
            return float4(0.0, 0.0, 0.0, 0.0);
        }
    }

    if (screenPos.x > topRight.x && screenPos.y < topRight.y) {
        float distance = distance(screenPos, topRight);
        if (distance < g_borderRadius.y) {
            float alpha = 1.0 - pow(distance / g_borderRadius.y, 50.0);
            return float4(g_backgroundColor.rgb, g_backgroundColor.a * alpha);
        } else {
            return float4(0.0, 0.0, 0.0, 0.0);
        }
    }

    if (screenPos.x < bottomLeft.x && screenPos.y > bottomLeft.y) {
        float distance = distance(screenPos, bottomLeft);
        if (distance < g_borderRadius.z) {
            float alpha = 1.0 - pow(distance / g_borderRadius.z, 50.0);
            return float4(g_backgroundColor.rgb, g_backgroundColor.a * alpha);
        } else {
            return float4(0.0, 0.0, 0.0, 0.0);
        }
    }

    if (screenPos.x > bottomRight.x && screenPos.y > bottomRight.y) {
        float distance = distance(screenPos, bottomRight);
        if (distance < g_borderRadius.w) {
            float alpha = 1.0 - pow(distance / g_borderRadius.w, 50.0);
            return float4(g_backgroundColor.rgb, g_backgroundColor.a * alpha);
        } else {
            return float4(0.0, 0.0, 0.0, 0.0);
        }
    }

    return g_backgroundColor;
}
