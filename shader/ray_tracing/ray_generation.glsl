#version 460
#extension GL_EXT_ray_tracing : require

#include "common.glsl"

layout(location = 0) rayPayloadEXT HitPayload payload;

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, rgba32f) uniform image2D outImage;
layout(binding = 2) uniform Ubo { GlobalUbo uni; };


void main()
{
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV        = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    const vec2 d           = inUV * 2.0 - 1.0;

    const vec4 origin    = uni.viewInverse * vec4(0, 0, 0, 1);
    const vec4 target    = uni.projInverse * vec4(d.x, d.y, 1, 1);
    const vec4 direction = uni.viewInverse * vec4(normalize(target.xyz), 0);

    const uint  rayFlags = gl_RayFlagsOpaqueEXT;
    const float tMin     = 0.001;
    const float tMax     = 10000.0;

    traceRayEXT(
        topLevelAS, // acceleration structure
        rayFlags, // rayFlags
        0xFF, // cullMask
        0, // sbtRecordOffset
        0, // sbtRecordStride
        0, // missIndex
        origin.xyz, // ray origin
        tMin, // ray min range
        direction.xyz, // ray direction
        tMax, // ray max range
        0// payload (location = 0)
    );

    imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), vec4(payload.hitValue, 1.0));
}
