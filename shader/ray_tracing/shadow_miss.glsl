#version 460
#extension GL_EXT_ray_tracing : require

#include "common.glsl"

layout(location = 1) rayPayloadInEXT bool isShadowed;

void main()
{
    isShadowed = false;
}
