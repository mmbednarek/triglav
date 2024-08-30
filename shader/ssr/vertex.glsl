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
} ubo;

layout(location = 0) out vec3 fragWorldPosition;
layout(location = 1) out vec3 fragViewPosition;
layout(location = 2) out vec3 fragWorldNormal;
layout(location = 3) out vec3 fragViewNormal;


void main() {
    vec4 worldSpace = ubo.model * vec4(inPosition, 1.0);
    vec4 viewSpace = ubo.view * worldSpace;

    fragWorldNormal = mat3(ubo.normal) * inNormal;
    fragViewNormal = mat3(ubo.view) * fragWorldNormal;
    fragWorldPosition = worldSpace.xyz;
    fragViewPosition = viewSpace.xyz;

    gl_Position = ubo.proj * viewSpace;

}
