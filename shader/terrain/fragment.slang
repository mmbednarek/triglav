import triglav.core;
import triglav.material;

[[vk::binding(4)]]
Sampler2D s_grass_texture;

[[vk::binding(5)]]
Sampler2D s_dirt_texture;

typealias FSOutput = triglav::material::FSOutput;

struct FSInput
{
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float blending : TEXCOORD1;
};

FSOutput fs_main(FSInput fs_in)
{
    float3 encoded_normal = triglav::encode_normal_unorm(normalize(fs_in.normal));
    float4 albedo_grass = s_grass_texture.Sample(fs_in.uv);
    float4 albedo_dirt = s_dirt_texture.Sample(fs_in.uv);
    float4 albedo = lerp(albedo_grass, albedo_dirt, fs_in.blending);

    FSOutput fs_out;
    fs_out.color = float4(albedo.xyz, 1.0);
    fs_out.material = float4(0.0, 0.0, 0, 1.0);
    fs_out.normal = float4(encoded_normal, 1.0);
    return fs_out;
}

