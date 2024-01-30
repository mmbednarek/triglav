#version 450

layout(location = 0) out vec2 fragTexCoord;

vec2 g_positions[4] = {
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, 1.0),
};

void main() {
    const vec2 point = g_positions[gl_VertexIndex];
    fragTexCoord = (1 + point.xy) * 0.5;
    gl_Position = vec4(point, 0.0, 1.0);
}
