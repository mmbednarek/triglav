import gauss_coefs;

#ifdef SINGLE_CHANNEL
#define PIXEL_TYPE float
#define FORMAT "r16f"
#else
#define PIXEL_TYPE float4
#define FORMAT "rgba16f"
#endif

[[vk::binding(0)]]
uniform Sampler2D<PIXEL_TYPE> inputImage;

[[vk::binding(1)]]
[vk::image_format(FORMAT)]
uniform RWTexture2D<PIXEL_TYPE> outputImage;

groupshared PIXEL_TYPE localPixel[1024];

PIXEL_TYPE read_pixel(const int2 globalPosition, const int2 size, const int2 localIndex, const int2 offset)
{
    int2 globalDisplaced = clamp(globalPosition + offset, int2(0), int2(size.x-1, size.y-1));
    int2 localPos = localIndex + (globalDisplaced - globalPosition);
    return localPixel[32 * localPos.y + localPos.x];
}

[numthreads(32, 32, 1)]
void cs_main(uint3 groupThreadIndex : SV_GroupThreadID, uint3 groupIndex : SV_GroupID) {
    int2 size;
    int numLayers;
    inputImage.GetDimensions(0, size.x, size.y, numLayers);

    uint2 localIndex = groupThreadIndex.xy;
    uint2 globalPosition = 16*groupIndex.xy + localIndex;

    if (globalPosition.x >= size.x || globalPosition.y >= size.y)
        return;

    let pixel = inputImage.Load(int3(globalPosition, 0));
    localPixel[32 * localIndex.y + localIndex.x] = pixel;

    AllMemoryBarrier();

    const int2 dimSize = int2(size.x - 8, size.y - 8);

    if (((globalPosition.x >= 8) && (localIndex.x < 8)) ||
        ((globalPosition.y >= 8) && (localIndex.y < 8)) ||
        ((globalPosition.x < dimSize.x) && (localIndex.x >= 24)) ||
        ((globalPosition.y < dimSize.y) && (localIndex.y >= 24)))
    {
        return;
    }

    int coefIndex = 0;
    var result = PIXEL_TYPE(0);
    for (int y = -7; y <= 7; ++y) {
        for (int x = -7; x <= 7; ++x) {
            float coef = g_gaussCoefficients[coefIndex];
            let pix = read_pixel(int2(globalPosition), size, int2(localIndex), int2(x, y));
            result += coef * pix;
            ++coefIndex;
        }
    }

    outputImage[int2(globalPosition)] = result;
}
