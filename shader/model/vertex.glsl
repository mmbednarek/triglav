#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normal;
    mat4 shadowMapMVP;
} ubo;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;
layout(location = 5) out vec4 fragShadowUV;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0 );

void main() {
    vec4 viewSpace = ubo.model * vec4(inPosition, 1.0);

    fragTexCoord = inTexCoord;
    fragNormal = mat3(ubo.normal) * inNormal;
    fragTangent = mat3(ubo.normal) * inTangent;
    fragBitangent = mat3(ubo.normal) * inBitangent;
    fragPosition = viewSpace.xyz;
    fragShadowUV = (biasMat * ubo.shadowMapMVP) * vec4(inPosition, 1.0);

    gl_Position = ubo.proj * ubo.view * viewSpace;
}