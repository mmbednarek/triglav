#version 450

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform SpriteUBO {
    mat3 transform;
} ubo;

vec3 g_positions[4] = {
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0),
    vec3(1.0, 0.0, 1.0),
    vec3(1.0, 1.0, 1.0),
};

void main() {
    const vec3 point = g_positions[gl_VertexIndex];
    fragTexCoord = point.xy;
    gl_Position = vec4(ubo.transform * point, 1.0);
}
