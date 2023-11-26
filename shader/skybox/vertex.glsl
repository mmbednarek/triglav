#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragPosition;

void main() {
    fragTexCoord = inTexCoord;
    fragPosition = inPosition;
    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
}
