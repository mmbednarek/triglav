#version 450

layout(location = 0) out vec4 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragViewPosition;

layout(binding = 0) uniform GroundUBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

vec4 g_positions[4] = {
    vec4(-1.0, 1.0, 0.0, 1.0),
    vec4(-1.0, -1.0, 0.0, 1.0),
    vec4(1.0, 1.0, 0.0, 1.0),
    vec4(1.0, -1.0, 0.0, 1.0),
};

void main() {
    vec4 position = g_positions[gl_VertexIndex];

    fragWorldPos = ubo.model * position;
    fragViewPosition = ubo.view * fragWorldPos;
    fragNormal = mat3(ubo.view) * vec3(0, 0, -1);

    gl_Position = ubo.proj * fragViewPosition;
}