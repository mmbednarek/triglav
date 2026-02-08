implementing core;

namespace triglav {

public typealias u32 = uint32_t;

public float3x3 invert_f3x3(const in float3x3 m) {
    float det = m._m00 * (m._m11 * m._m22 - m._m21 * m._m12) -
                 m._m01 * (m._m10 * m._m22 - m._m12 * m._m20) +
                 m._m02 * (m._m10 * m._m21 - m._m11 * m._m20);

    float invdet = 1 / det;

    float3x3 result;
    result._m00 = (m._m11 * m._m22 - m._m21 * m._m12) * invdet;
    result._m01 = (m._m02 * m._m21 - m._m01 * m._m22) * invdet;
    result._m02 = (m._m01 * m._m12 - m._m02 * m._m11) * invdet;
    result._m10 = (m._m12 * m._m20 - m._m10 * m._m22) * invdet;
    result._m11 = (m._m00 * m._m22 - m._m02 * m._m20) * invdet;
    result._m12 = (m._m10 * m._m02 - m._m00 * m._m12) * invdet;
    result._m20 = (m._m10 * m._m21 - m._m20 * m._m11) * invdet;
    result._m21 = (m._m20 * m._m01 - m._m00 * m._m21) * invdet;
    result._m22 = (m._m00 * m._m11 - m._m10 * m._m01) * invdet;

    return result;
}

public float3x3 normal_transform_matrix(const in float4x4 matrix) {
    return transpose(invert_f3x3(float3x3(matrix)));
}

public struct Transform
{
    public float4 rotation;
    public float3 scale;
    public float3 translation;

    public float4x4 matrix()
    {
        // scale
        const float4x4 scale = float4x4(
            scale.x,    0,    0,       0,
            0,       scale.y, 0,       0,
            0,       0,       scale.z, 0,
            0,       0,       0,       1);

        // rotation
        const float4x4 rotation = float4x4(
            cos(rotation.w), -sin(rotation.w), 0, 0,
            sin(rotation.w), cos(rotation.w),  0, 0,
            0,               0,                1, 0,
            0,               0,                0, 1);

        // translation
        const float4x4 translation = float4x4(
            1, 0, 0, translation.x,
            0, 1, 0, translation.y,
            0, 0, 1, translation.z,
            0, 0, 0, 1,
        );

        return mul(mul(scale, rotation), translation);
    }
};

public struct BoundingBox
{
    public float3 min;
    public float3 max;
};

}
