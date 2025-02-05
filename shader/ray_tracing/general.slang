import ray_tracing;
import ray_payload;

static constexpr uint AMBIENT_OCCLUSION_RAY_COUNT = 8;
static constexpr float AMBIENT_OCCLUSION_DISTANCE = 4.0;

[[vk::binding(0)]]
uniform RaytracingAccelerationStructure topLevelAS : register(t0);

[[vk::binding(4)]]
StructuredBuffer<ray_tracing.ObjectData> objects;

struct PushConstants
{
    float3 lightDir;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

uint lcg(inout uint prev)
{
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = (LCG_A * prev + LCG_C);
  return prev & 0x00FFFFFF;
}

float rnd(inout uint seed)
{
  return (float(lcg(seed)) / float(0x01000000));
}

static const float3 PrimeVector = float3(74929, 54851, 85847);


[shader("closesthit")]
void chit_main(inout ray_payload.GeneralOutput payload, in ray_tracing.Attributes attribs)
{
    // Read object info
    let object = objects[InstanceIndex()];
    let hitInfo = ray_tracing.ObjectHitInfo(object, PrimitiveIndex(), attribs, ObjectToWorld3x4());

    let lightDir = -normalize(pc.lightDir);

    // Trace a shadow, offset the origin by the normal just slightly to avoid artifacts.
    RayDesc rayDesc;
    rayDesc.Origin = hitInfo.worldSpacePosition + 0.05*hitInfo.normal;
    rayDesc.Direction = lightDir;
    rayDesc.TMin = 0.15;
    rayDesc.TMax = 1000.0;

    ray_payload.Shadow shadowPayload;
    shadowPayload.isShadowed = false;

    TraceRay(
        topLevelAS,
        RAY_FLAG_FORCE_OPAQUE,
        0xFF,
        1,
        0,
        1,
        rayDesc,
        shadowPayload
    );

    payload.shadowTerm = shadowPayload.isShadowed ? 0.0 : 1.0;

    // Calculate ambient occlusion term

    uint seed = uint(abs(dot(hitInfo.worldSpacePosition, PrimeVector)));

    float distanceSum = 0;
    for (int i = 0; i < AMBIENT_OCCLUSION_RAY_COUNT; ++i) {
        let randomVector = normalize(float3(rnd(seed), rnd(seed), rnd(seed)));
        let tangent = normalize(randomVector - hitInfo.normal * dot(randomVector, hitInfo.normal));
        let bitangent = cross(hitInfo.normal, tangent);

        float3x3 tgSpaceMat = float3x3(
            tangent,
            bitangent,
            hitInfo.normal,
        );

        let aoPoint = normalize(float3(2*rnd(seed)-1, 2*rnd(seed)-1, rnd(seed)));

        float3 traceDir = mul(transpose(tgSpaceMat), aoPoint);
        traceDir += 0.01 * hitInfo.normal;

        RayDesc aoRayDesc;
        aoRayDesc.Origin = hitInfo.worldSpacePosition;
        aoRayDesc.Direction = traceDir;
        aoRayDesc.TMin = 0.005;
        aoRayDesc.TMax = AMBIENT_OCCLUSION_DISTANCE;

        ray_payload.AmbientOcclusion aoPayload;
        TraceRay(
            topLevelAS,
            RAY_FLAG_FORCE_OPAQUE,
            0xFF,
            2,
            0,
            2,
            aoRayDesc,
            aoPayload
        );

        distanceSum += aoPayload.distance / AMBIENT_OCCLUSION_DISTANCE;

    }

    distanceSum /= float(AMBIENT_OCCLUSION_RAY_COUNT);
    payload.ambientOcclusionTerm = pow(clamp(2*distanceSum, 0, 1), 2);
}

[shader("miss")]
void miss_main(inout ray_payload.GeneralOutput payload)
{
    payload.shadowTerm = 1.0;
    payload.ambientOcclusionTerm = 0.0;
}