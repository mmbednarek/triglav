#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 viewProj;
} ubo;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPosition;

void main() {
    fragTexCoord = inTexCoord;
    fragNormal = mat3(ubo.model) * inNormal;
    fragPosition = (ubo.model * vec4(inPosition, 1.0)).xyz;
    gl_Position = ubo.viewProj * ubo.model * vec4(inPosition, 1.0);
}