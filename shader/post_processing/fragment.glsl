#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texOverlay;

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

const float g_gaussCoefs[] = {
    7.053342846020118e-09,
    7.73492964270981e-06,
    0.001147965343710727,
    0.023057500297641625,
    0.06267678406876825,
    0.023057500297641625,
    0.001147965343710727,
    7.73492964270981e-06,
    7.053342846020118e-09,
    7.053719545226337e-09,
    7.735342743550106e-06,
    0.0011480266533114628,
    0.02305873173389095,
    0.0626801314595477,
    0.02305873173389095,
    0.0011480266533114628,
    7.735342743550106e-06,
    7.053719545226337e-09,
    7.053988628405276e-09,
    7.735637829086505e-06,
    0.0011480704478881254,
    0.023059611371477544,
    0.06268252256241497,
    0.023059611371477544,
    0.0011480704478881254,
    7.735637829086505e-06,
    7.053988628405276e-09,
    7.054150083239768e-09,
    7.7358148858116e-06,
    0.0011480967254360367,
    0.023060139170136367,
    0.06268395726791832,
    0.023060139170136367,
    0.0011480967254360367,
    7.7358148858116e-06,
    7.054150083239768e-09,
    7.054203902339138e-09,
    7.735873905620525e-06,
    0.0011481054847523292,
    0.02306031510570718,
    0.06268443551038345,
    0.02306031510570718,
    0.0011481054847523292,
    7.735873905620525e-06,
    7.054203902339138e-09,
    7.054150083239768e-09,
    7.7358148858116e-06,
    0.0011480967254360367,
    0.023060139170136367,
    0.06268395726791832,
    0.023060139170136367,
    0.0011480967254360367,
    7.7358148858116e-06,
    7.054150083239768e-09,
    7.053988628405276e-09,
    7.735637829086505e-06,
    0.0011480704478881254,
    0.023059611371477544,
    0.06268252256241497,
    0.023059611371477544,
    0.0011480704478881254,
    7.735637829086505e-06,
    7.053988628405276e-09,
    7.053719545226337e-09,
    7.735342743550106e-06,
    0.0011480266533114628,
    0.02305873173389095,
    0.0626801314595477,
    0.02305873173389095,
    0.0011480266533114628,
    7.735342743550106e-06,
    7.053719545226337e-09,
    7.053342846020118e-09,
    7.73492964270981e-06,
    0.001147965343710727,
    0.023057500297641625,
    0.06267678406876825,
    0.023057500297641625,
    0.001147965343710727,
    7.73492964270981e-06,
    7.053342846020118e-09
};

vec3 sample_blurred_color(vec2 coord) {
    ivec2 texSize = textureSize(texColor, 0);
    vec2 pixelOffset = vec2(1.0 / texSize.x, 1.0 / texSize.y);

    vec3 result = vec3(0.0);
    int coefIndex = 0;
    for (int y = -4; y <= 4; ++y) {
        for (int x = -4; x <= 4; ++x) {
            float coef = g_gaussCoefs[coefIndex];
            result += coef * texture(texColor, coord + vec2(x * pixelOffset.x, y * pixelOffset.y)).rgb;
            ++coefIndex;
        }
    }

    return result;
}

void main() {

    vec3 backColor;
    if (pc.enableFXAA) {
        backColor  = calculate_fxaa();
    } else {
        backColor = texture(texColor, fragTexCoord).rgb;
    }

    if (pc.hideUI) {
        outColor = vec4(backColor, 1.0);
    } else {
        vec4 overlayColor = texture(texOverlay, fragTexCoord);
        outColor = vec4(overlayColor.rgb * overlayColor.a + backColor * (1 - overlayColor.a), 1.0);
    }
}