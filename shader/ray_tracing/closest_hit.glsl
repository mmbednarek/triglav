#version 460
#extension GL_EXT_ray_tracing : require

#include "common.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT HitPayload prd;

// layout(binding = 0) uniform accelerationStructureEXT topLevelAS;

void main()
{
    prd.hitValue = vec3(1.0, 0.5, 0.5);
}
