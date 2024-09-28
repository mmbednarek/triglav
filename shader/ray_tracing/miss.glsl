#version 460
#extension GL_EXT_ray_tracing : require

#include "common.glsl"

layout(location = 0) rayPayloadInEXT HitPayload prd;
layout(location = 1) rayPayloadEXT HitPayload refl;

void main()
{
    prd.hitValue = vec3(0.05, 0.05, 0.1);
    refl.hitValue = vec3(0.05, 0.05, 0.1);
}
