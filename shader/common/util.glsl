#ifndef UTIL_H
#define UTIL_H

float linearize_depth(float depth)
{
    float n = 0.1;
    float f = 200.0;
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}

vec3 srgb_to_linear(vec3 inColor)
{
    return pow(inColor, vec3(2.2));
}

vec3 linear_to_srgb(vec3 inColor)
{
    return pow(inColor, vec3(1.0/2.2));
}

#endif // UTIL_H
