#version 450

#include "../common/util.glsl"

const float GAUSS_COEFS[] = {
    0.0003017204601892108,
    0.0004982212074564017,
    0.000761604114671245,
    0.0010777697173198154,
    0.0014119271690739255,
    0.001712333041407348,
    0.0019224444610764014,
    0.001998062138288006,
    0.0019224444610764014,
    0.001712333041407348,
    0.0014119271690739255,
    0.0010777697173198154,
    0.000761604114671245,
    0.0004982212074564017,
    0.0003017204601892108,
    0.0004982212074564017,
    0.0008226965165161549,
    0.001257612166497817,
    0.0017796861690661857,
    0.002331469528368647,
    0.0028275199995468126,
    0.0031744701703845745,
    0.003299335187565671,
    0.0031744701703845745,
    0.0028275199995468126,
    0.002331469528368647,
    0.0017796861690661857,
    0.001257612166497817,
    0.0008226965165161549,
    0.0004982212074564017,
    0.000761604114671245,
    0.001257612166497817,
    0.0019224444610764014,
    0.002720511067973601,
    0.0035639927796360864,
    0.0043222786058510635,
    0.004852642776908663,
    0.005043517250817807,
    0.004852642776908663,
    0.0043222786058510635,
    0.0035639927796360864,
    0.002720511067973601,
    0.0019224444610764014,
    0.001257612166497817,
    0.000761604114671245,
    0.0010777697173198154,
    0.0017796861690661857,
    0.002720511067973601,
    0.0038498799943603294,
    0.005043517250817807,
    0.006116591154731938,
    0.006867126021477044,
    0.007137238437922464,
    0.006867126021477044,
    0.006116591154731938,
    0.005043517250817807,
    0.0038498799943603294,
    0.002720511067973601,
    0.0017796861690661857,
    0.0010777697173198154,
    0.0014119271690739255,
    0.002331469528368647,
    0.0035639927796360864,
    0.005043517250817807,
    0.0066072361467265075,
    0.00801301158744711,
    0.008996246273544941,
    0.009350105779295303,
    0.008996246273544941,
    0.00801301158744711,
    0.0066072361467265075,
    0.005043517250817807,
    0.0035639927796360864,
    0.002331469528368647,
    0.0014119271690739255,
    0.001712333041407348,
    0.0028275199995468126,
    0.0043222786058510635,
    0.006116591154731938,
    0.00801301158744711,
    0.009717884040268956,
    0.010910314696283157,
    0.011339462415077918,
    0.010910314696283157,
    0.009717884040268956,
    0.00801301158744711,
    0.006116591154731938,
    0.0043222786058510635,
    0.0028275199995468126,
    0.001712333041407348,
    0.0019224444610764014,
    0.0031744701703845745,
    0.004852642776908663,
    0.006867126021477044,
    0.008996246273544941,
    0.010910314696283157,
    0.012249062273091063,
    0.01273086846092383,
    0.012249062273091063,
    0.010910314696283157,
    0.008996246273544941,
    0.006867126021477044,
    0.004852642776908663,
    0.0031744701703845745,
    0.0019224444610764014,
    0.001998062138288006,
    0.003299335187565671,
    0.005043517250817807,
    0.007137238437922464,
    0.009350105779295303,
    0.011339462415077918,
    0.01273086846092383,
    0.013231626075197128,
    0.01273086846092383,
    0.011339462415077918,
    0.009350105779295303,
    0.007137238437922464,
    0.005043517250817807,
    0.003299335187565671,
    0.001998062138288006,
    0.0019224444610764014,
    0.0031744701703845745,
    0.004852642776908663,
    0.006867126021477044,
    0.008996246273544941,
    0.010910314696283157,
    0.012249062273091063,
    0.01273086846092383,
    0.012249062273091063,
    0.010910314696283157,
    0.008996246273544941,
    0.006867126021477044,
    0.004852642776908663,
    0.0031744701703845745,
    0.0019224444610764014,
    0.001712333041407348,
    0.0028275199995468126,
    0.0043222786058510635,
    0.006116591154731938,
    0.00801301158744711,
    0.009717884040268956,
    0.010910314696283157,
    0.011339462415077918,
    0.010910314696283157,
    0.009717884040268956,
    0.00801301158744711,
    0.006116591154731938,
    0.0043222786058510635,
    0.0028275199995468126,
    0.001712333041407348,
    0.0014119271690739255,
    0.002331469528368647,
    0.0035639927796360864,
    0.005043517250817807,
    0.0066072361467265075,
    0.00801301158744711,
    0.008996246273544941,
    0.009350105779295303,
    0.008996246273544941,
    0.00801301158744711,
    0.0066072361467265075,
    0.005043517250817807,
    0.0035639927796360864,
    0.002331469528368647,
    0.0014119271690739255,
    0.0010777697173198154,
    0.0017796861690661857,
    0.002720511067973601,
    0.0038498799943603294,
    0.005043517250817807,
    0.006116591154731938,
    0.006867126021477044,
    0.007137238437922464,
    0.006867126021477044,
    0.006116591154731938,
    0.005043517250817807,
    0.0038498799943603294,
    0.002720511067973601,
    0.0017796861690661857,
    0.0010777697173198154,
    0.000761604114671245,
    0.001257612166497817,
    0.0019224444610764014,
    0.002720511067973601,
    0.0035639927796360864,
    0.0043222786058510635,
    0.004852642776908663,
    0.005043517250817807,
    0.004852642776908663,
    0.0043222786058510635,
    0.0035639927796360864,
    0.002720511067973601,
    0.0019224444610764014,
    0.001257612166497817,
    0.000761604114671245,
    0.0004982212074564017,
    0.0008226965165161549,
    0.001257612166497817,
    0.0017796861690661857,
    0.002331469528368647,
    0.0028275199995468126,
    0.0031744701703845745,
    0.003299335187565671,
    0.0031744701703845745,
    0.0028275199995468126,
    0.002331469528368647,
    0.0017796861690661857,
    0.001257612166497817,
    0.0008226965165161549,
    0.0004982212074564017,
    0.0003017204601892108,
    0.0004982212074564017,
    0.000761604114671245,
    0.0010777697173198154,
    0.0014119271690739255,
    0.001712333041407348,
    0.0019224444610764014,
    0.001998062138288006,
    0.0019224444610764014,
    0.001712333041407348,
    0.0014119271690739255,
    0.0010777697173198154,
    0.000761604114671245,
    0.0004982212074564017,
    0.0003017204601892108
};

#ifdef SINGLE_CHANNEL
#define PIXEL_TYPE float
#define SUFFIX .r
#define OUTPUT(result) vec4(result, 0, 0, 0)
#define FORMAT r16f
#else
#define PIXEL_TYPE vec4
#define SUFFIX
#define OUTPUT(result) result
#define FORMAT rgba16f
#endif

layout(binding = 0) uniform sampler2D inputImage;

layout(binding = 1, FORMAT) uniform image2D outputImage;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

shared PIXEL_TYPE localPixel[1024];

PIXEL_TYPE read_pixel(const ivec2 globalPosition, const ivec2 size, const ivec2 localIndex, const ivec2 offset)
{
    ivec2 globalDisplaced = clamp(globalPosition + offset, ivec2(0), ivec2(size.x-1, size.y-1));
    ivec2 localPos = localIndex + (globalDisplaced - globalPosition);
    return localPixel[32 * localPos.y + localPos.x];
}

void main() {
    const ivec2 size = textureSize(inputImage, 0);
    uvec2 localIndex = gl_LocalInvocationID.xy;
    uvec2 globalPosition = 16*gl_WorkGroupID.xy + localIndex;

    if (globalPosition.x >= size.x || globalPosition.y >= size.y)
        return;

    PIXEL_TYPE pixel = texelFetch(inputImage, ivec2(globalPosition), 0) SUFFIX;
    localPixel[32 * localIndex.y + localIndex.x] = pixel;

    barrier();

    const ivec2 dimSize = ivec2(size.x - 8, size.y - 8);

    if (((globalPosition.x >= 8) && (localIndex.x < 8)) ||
        ((globalPosition.y >= 8) && (localIndex.y < 8)) ||
        ((globalPosition.x < dimSize.x) && (localIndex.x >= 24)) ||
        ((globalPosition.y < dimSize.y) && (localIndex.y >= 24)))
    {
        return;
    }

    int coefIndex = 0;
    PIXEL_TYPE result = PIXEL_TYPE(0);
    for (int y = -7; y <= 7; ++y) {
        for (int x = -7; x <= 7; ++x) {
            float coef = GAUSS_COEFS[coefIndex];
            PIXEL_TYPE pix = read_pixel(ivec2(globalPosition), size, ivec2(localIndex), ivec2(x, y));
            result += coef * pix;
            ++coefIndex;
        }
    }

    imageStore(outputImage, ivec2(globalPosition), OUTPUT(result));
}