#version 450

layout(location = 0) in vec2 point;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform SpriteUBO {
    mat3 transform;
} ubo;

void main() {
    fragTexCoord = texCoord;
    gl_Position = vec4(ubo.transform * vec3(point, 1.0), 1.0);
}