#version 450

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform SpriteUBO {
    mat4 transform;
} ubo;

vec4 g_positions[4] = {
    vec4(0.0, 0.0, 0.0, 1.0),
    vec4(0.0, 1.0, 0.0, 1.0),
    vec4(1.0, 0.0, 0.0, 1.0),
    vec4(1.0, 1.0, 0.0, 1.0),
};

void main() {
    const vec4 point = g_positions[gl_VertexIndex];
    fragTexCoord = point.xy;
    gl_Position = ubo.transform * point;
}
