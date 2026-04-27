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

public float4x4 translation_to_matrix(float3 translation) {
    return float4x4(
        1, 0, 0, translation.x,
        0, 1, 0, translation.y,
        0, 0, 1, translation.z,
        0, 0, 0, 1,
    );
}

public float4x4 scale_to_matrix(float3 scale) {
    return float4x4(
        scale.x,    0,    0,       0,
        0,       scale.y, 0,       0,
        0,       0,       scale.z, 0,
        0,       0,       0,       1);
}

public float4x4 quaterion_rot_to_matrix(float4 quat) {
    float4 q = normalize(quat);
    return float4x4(
        1.0 - 2.0*q.y*q.y - 2.0*q.z*q.z, 2.0*q.x*q.y - 2.0*q.z*q.w,       2.0*q.x*q.z + 2.0*q.y*q.w,       0.0,
        2.0*q.x*q.y + 2.0*q.z*q.w,       1.0 - 2.0*q.x*q.x - 2.0*q.z*q.z, 2.0*q.y*q.z - 2.0*q.x*q.w,       0.0,
        2.0*q.x*q.z - 2.0*q.y*q.w,       2.0*q.y*q.z + 2.0*q.x*q.w,       1.0 - 2.0*q.x*q.x - 2.0*q.y*q.y, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

public float4x4 extent_mat3x3(float3x3 matrix) {
    return float4x4(
        matrix._m00, matrix._m01, matrix._m02, 0.0,
        matrix._m10, matrix._m11, matrix._m12, 0.0,
        matrix._m20, matrix._m21, matrix._m22, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

public struct Transform
{
    public float4 rotation;
    public float3 scale;
    public float3 translation;

    public float4x4 matrix()
    {
        const float4x4 scale = scale_to_matrix(scale);
        const float4x4 rotation = quaterion_rot_to_matrix(rotation);
        const float4x4 translation = translation_to_matrix(translation);
        return mul(translation, mul(rotation, scale));
    }
};

public struct BoundingBox
{
    public float3 min;
    public float3 max;
};

}
