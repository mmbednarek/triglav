module shadow_map;

namespace triglav::shadow_map
{
    static const int PCR_FILTER_RANGE = 2;
    static const int PCR_FILTER_KERNEL_COUNT = (2*PCR_FILTER_RANGE + 1) * (2*PCR_FILTER_RANGE + 1);
    static const float PCR_SCALE = 1.5;

    public bool test_bounds(float4 shadowCoord)
    {
        return shadowCoord.z > -0.99 && shadowCoord.z < 0.99 &&
            shadowCoord.y > 0.01 && shadowCoord.y < 0.99 &&
            shadowCoord.x > 0.01 && shadowCoord.x < 0.99;
    }

    public float test(Sampler2D<float> shadowMap, float4 shadowCoord, float2 off, float bias)
    {
        float shadow = 1.0;

        if (test_bounds(shadowCoord)) {
            float dist = shadowMap.Sample(shadowCoord.xy + off).r;

            if (shadowCoord.w > 0.0 && dist < shadowCoord.z - bias) {
                shadow = 0;
            }
        }

        return shadow;
    }

    public float test_pcr(Sampler2D<float> shadowMap, float4 shadowCoord, float bias)
    {
        uint width, height, numLevels;
        shadowMap.GetDimensions(0, width, height, numLevels);
        int2 texDim = int2(width, height);

        float dx = PCR_SCALE * 1.0 / float(texDim.x);
        float dy = PCR_SCALE * 1.0 / float(texDim.y);

        float shadowFactor = 0.0;

        for (int x = -PCR_FILTER_RANGE; x <= PCR_FILTER_RANGE; x++) {
            for (int y = -PCR_FILTER_RANGE; y <= PCR_FILTER_RANGE; y++) {
                shadowFactor += test(shadowMap, shadowCoord, float2(dx*x, dy*y), bias);
            }
        }

        return shadowFactor / PCR_FILTER_KERNEL_COUNT;
    }
}