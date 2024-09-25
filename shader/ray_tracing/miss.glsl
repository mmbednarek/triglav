#version 460
#extension GL_EXT_ray_tracing : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;

void main()
{
    prd.hitValue = vec3(0.5, 0.0, 1.0);
}
