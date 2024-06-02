#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texOverlay;

#include "../common/blur.glsl"

const vec3 luma = vec3(0.299, 0.587, 0.114);

float getLuma(vec2 offset) {
    return dot(luma, texture(texColor, fragTexCoord + offset).rgb);
}

const float g_fxxaSpanMax = 8.0;
const float g_fxaaReduceMin = 1.0/128.0;
const float g_fxaaReduceMul = 1.0/8.0;
const float g_fxaaSubpixelShift = 1.0/4.0;

layout(push_constant) uniform Constants
{
    bool enableFXAA;
    bool hideUI;
} pc;

float linearize_depth(float depth)
{
    float n = 0.1;
    float f = 200.0;
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}

vec3 calculate_fxaa()
{
    ivec2 texDimensions = textureSize(texColor, 0);
    vec2 pixelOffset = vec2(1.0 / float(texDimensions.x), 1.0 / float(texDimensions.y));

    vec4 uv = vec4( fragTexCoord, fragTexCoord - (pixelOffset * (0.5 + g_fxaaSubpixelShift)));

    vec3 rgbNW = textureLod(texColor, uv.zw, 0.0).xyz;
    vec3 rgbNE = textureLod(texColor, uv.zw + vec2(1,0)*pixelOffset.xy, 0.0).xyz;
    vec3 rgbSW = textureLod(texColor, uv.zw + vec2(0,1)*pixelOffset.xy, 0.0).xyz;
    vec3 rgbSE = textureLod(texColor, uv.zw + vec2(1,1)*pixelOffset.xy, 0.0).xyz;
    vec3 rgbM  = textureLod(texColor, uv.xy, 0.0).xyz;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * g_fxaaReduceMul), g_fxaaReduceMin);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(g_fxxaSpanMax, g_fxxaSpanMax), max(vec2(-g_fxxaSpanMax, -g_fxxaSpanMax), dir * rcpDirMin)) * pixelOffset.xy;

    vec3 rgbA = (1.0/2.0) * (
        textureLod(texColor, uv.xy + dir * (1.0/3.0 - 0.5), 0.0).xyz +
        textureLod(texColor, uv.xy + dir * (2.0/3.0 - 0.5), 0.0).xyz
    );

    vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
        textureLod(texColor, uv.xy + dir * (0.0/3.0 - 0.5), 0.0).xyz +
        textureLod(texColor, uv.xy + dir * (3.0/3.0 - 0.5), 0.0).xyz
    );

    float lumaB = dot(rgbB, luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax))
        return rgbA;
    return rgbB;
}

void main() {
    vec3 backColor;
    if (pc.enableFXAA) {
        backColor  = calculate_fxaa();
    } else {
        backColor = blur_image(texColor, fragTexCoord);
//        backColor = texture(texColor, fragTexCoord).rgb;
    }

    if (pc.hideUI) {
        outColor = vec4(backColor, 1.0);
    } else {
        vec4 overlayColor = texture(texOverlay, fragTexCoord);
        outColor = vec4(overlayColor.rgb * overlayColor.a + backColor * (1 - overlayColor.a), 1.0);
    }
}