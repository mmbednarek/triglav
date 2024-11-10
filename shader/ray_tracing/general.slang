import ray_tracing;
import ray_payload;

static constexpr uint AMBIENT_OCCLUSION_RAY_COUNT = 16;
static constexpr float AMBIENT_OCCLUSION_DISTANCE = 4.0;

[[vk::binding(0)]]
uniform RaytracingAccelerationStructure topLevelAS : register(t0);

[[vk::binding(3)]]
StructuredBuffer<ray_tracing.ObjectData> objects;

struct AmbientOcclusionPoints
{
    float4 points[64];
};
[[vk::binding(4)]]
ConstantBuffer<AmbientOcclusionPoints> ambientOccPoints;

struct PushConstants
{
    float3 lightDir;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

static const float3 COLORS[] = {
    float3(0.5, 1.0, 1.0),
    float3(1.0, 0.5, 1.0),
    float3(1.0, 1.0, 0.5),
    float3(1.0, 0.5, 0.5),
    float3(0.5, 1.0, 0.5),
    float3(0.5, 0.5, 1.0),
};

uint lcg(inout uint prev)
{
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = (LCG_A * prev + LCG_C);
  return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint seed)
{
  return (float(lcg(seed)) / float(0x01000000));
}

static const float3 PrimeVector = float3(74929, 54851, 85847);


[shader("closesthit")]
void chit_main(inout ray_payload.Color payload, in ray_tracing.Attributes attribs)
{
    // Read object info
    let object = objects[InstanceIndex()];
    let hitInfo = ray_tracing.ObjectHitInfo(object, PrimitiveIndex(), attribs, ObjectToWorld3x4());

    let lightDir = -normalize(pc.lightDir);

    let dotProd = dot(hitInfo.normal, lightDir);
    // let dotProd = 1.0;
    // float lightVal = 0.2 + max(dotProd, 0);
    float lightVal = 1.0;

    bool isInShadow = true;


    if (true) {
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
        // shadowPayload.isShadowed = true;

        if (shadowPayload.isShadowed) {
            lightVal = 0.2;
            isInShadow = true;
        }
    } else {
        isInShadow = true;
    }

    if (isInShadow) {
        uint seed = uint(abs(dot(hitInfo.worldSpacePosition, PrimeVector)));
        // uint seed = PrimitiveIndex() * uint(1000*attribs.barycentrics.x) * uint(1000*attribs.barycentrics.y);

        float distanceSum = 0;
        int occludedCount = 0;
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

            // float3 traceDir = mul(transpose(tgSpaceMat), ambientOccPoints.points[i].xyz);
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
        // lightVal *= clamp(2*distanceSum, 0, 1);
        lightVal = clamp(2*distanceSum, 0, 1);
        // lightVal *= 0.2;
    }

    // let outColor = COLORS[InstanceIndex() % 6];
    let outColor = float3(0.1) + hitInfo.worldSpacePosition / 60.0;
    payload.hitValue = outColor * (lightVal);
}

[shader("miss")]
void miss_main(inout ray_payload.Color payload)
{
    payload.hitValue = float3(0.2, 0.0, 0.1);
}