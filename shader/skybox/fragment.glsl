#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragPosition;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

const float oneOverTwoPi = 0.1591550;
const float oneOverPi    = 0.3183099;
const float pi     = 3.1415927;
const float halfPi = 1.5707963;

void main() {
    vec3 normal = normalize(fragPosition);
    float yaw = oneOverTwoPi * (pi + atan(normal.y, normal.x));
    float pitch = oneOverPi * (halfPi + asin(normal.z));
    vec2 uv = vec2(yaw, pitch);

    outColor = texture(texSampler, uv);
}
