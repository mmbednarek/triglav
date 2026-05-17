import triglav.core;
import triglav.material;

typealias FSOutput = triglav::material::FSOutput;

struct FSInput
{
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

FSOutput fs_main(FSInput fs_in)
{
    float3 encoded_normal = triglav::encode_normal_unorm(normalize(fs_in.normal));

    FSOutput fs_out;
    fs_out.color = float4(0.2, 0.2, 1.0, 1.0);
    fs_out.material = float4(1.0, 0.0, 0, 1.0);
    fs_out.normal = float4(encoded_normal, 1.0);
    return fs_out;
}

