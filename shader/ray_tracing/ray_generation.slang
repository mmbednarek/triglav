import triglav.core;
import ray_tracing;
import ray_payload;

[[vk::binding(0)]]
uniform RaytracingAccelerationStructure topLevelAS;

[[vk::binding(1)]]
[vk::image_format("r16f")]
uniform RWTexture2D<float> outAmbientOcclusionImage : register(u0);

[[vk::binding(2)]]
[vk::image_format("r16f")]
uniform RWTexture2D<float> outShadowsImage : register(u1);

[[vk::binding(3)]]
uniform ConstantBuffer<triglav::ViewProperties> transform;

[shader("raygeneration")]
void rgen_main()
{
    let pixelCenter = float2(DispatchRaysIndex().xy) + float2(0.5);
    let inUV        = pixelCenter / float2(DispatchRaysDimensions().xy);
    let d           = inUV * 2.0 - 1.0;

    let target    = mul(transform.invProj, float4(d.x, d.y, 1, 1));

    RayDesc rayDesc;
    rayDesc.Origin    = mul(transform.invView, float4(0, 0, 0, 1)).xyz;
    rayDesc.Direction = mul(transform.invView, float4(normalize(target.xyz), 0)).xyz;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000.0;

    ray_payload.GeneralOutput outPayload;

    TraceRay(
        topLevelAS,
        RAY_FLAG_FORCE_OPAQUE,
        0xFF,
        0,
        0,
        0,
        rayDesc,
        outPayload
    );

    outAmbientOcclusionImage[DispatchRaysIndex().xy] = outPayload.ambientOcclusionTerm;
    outShadowsImage[DispatchRaysIndex().xy] = outPayload.shadowTerm;
}
