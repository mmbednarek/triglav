#version 450

struct FragIn
{
    [[vk::location(0)]]
    float2 texCoord : TEXCOORD0;
};

struct FragOut
{
    [[vk::location(0)]]
    float4 color : SV_Target0;
};

[[vk::binding(0)]]
uniform Sampler2D texColor;
[[vk::binding(1)]]
uniform Sampler2D texBloom;
[[vk::binding(2)]]
uniform Sampler2D texOverlay;

const static float3 g_luma = float3(0.299, 0.587, 0.114);

float getLuma(float2 texCoord, float2 offset)
{
    return dot(g_luma, texColor.Sample(texCoord + offset).xyz);
}

const static float g_fxxaSpanMax = 8.0;
const static float g_fxaaReduceMin = 1.0/128.0;
const static float g_fxaaReduceMul = 1.0/8.0;
const static float g_fxaaSubpixelShift = 1.0/4.0;

struct PushConstants
{
    bool enableFXAA;
    bool hideUI;
    bool enableBloom;
};

[[vk::push_constant]]
uniform ConstantBuffer<PushConstants> pc;

vec3 calculate_fxaa(float2 texCoord)
{
    uint2 texDimensions;
    uint lvlCount;
    texColor.GetDimensions(0, texDimensions.x, texDimensions.y, lvlCount);

    int3 pixelPos = int3(int2(texCoord * float2(texDimensions)), 0);

    float2 pixelOffset = float2(1.0 / float(texDimensions.x), 1.0 / float(texDimensions.y));
    float4 uv = float4(texCoord, texCoord - (pixelOffset * (0.5 + g_fxaaSubpixelShift)));

    float3 rgbNW = texColor.Sample(uv.zw).xyz;
    float3 rgbNE = texColor.Sample(uv.zw + pixelOffset*float2(1, 0)).xyz;
    float3 rgbSW = texColor.Sample(uv.zw + pixelOffset*float2(0, 1)).xyz;
    float3 rgbSE = texColor.Sample(uv.zw + pixelOffset*float2(1, 1)).xyz;
    float3 rgbM = texColor.Sample(uv.xy).xyz;

    const float3 luma = float3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * g_fxaaReduceMul), g_fxaaReduceMin);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(float2(g_fxxaSpanMax, g_fxxaSpanMax), max(float2(-g_fxxaSpanMax, -g_fxxaSpanMax), dir * rcpDirMin)) * pixelOffset;

    float3 rgbA = (1.0/2.0) * (
        texColor.Sample(uv.xy + dir * (1.0/3.0 - 0.5)).xyz +
        texColor.Sample(uv.xy + dir * (2.0/3.0 - 0.5)).xyz
    );

    float3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        texColor.Sample(uv.xy + dir * (0.0/3.0 - 0.5)).xyz +
        texColor.Sample(uv.xy + dir * (3.0/3.0 - 0.5)).xyz
    );

    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax))
        return rgbA;

    return rgbB;
}

FragOut fs_main(FragIn fragIn) {
    float3 backColor;
    if (pc.enableFXAA) {
        backColor = calculate_fxaa(fragIn.texCoord);
    } else {
        backColor = texColor.Sample(fragIn.texCoord).rgb;
    }

    float4 bloomColor = texBloom.Sample(fragIn.texCoord);
    if (pc.enableBloom) {
        backColor = backColor*(1.0 - bloomColor.a) + bloomColor.rgb*bloomColor.a;
    }

    FragOut output;
    if (pc.hideUI) {
        output.color = float4(backColor, 1.0);
    } else {
        float4 overlayColor = texOverlay.Sample(fragIn.texCoord);
        output.color = float4(overlayColor.rgb * overlayColor.a + backColor * (1 - overlayColor.a), 1.0);
    }

    return output;
}
