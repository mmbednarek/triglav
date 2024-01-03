#version 450

layout(binding = 1) uniform sampler2D fontAtlas;

layout(location = 0) in vec2 fragTexCoord;

layout(push_constant) uniform Constants
{
    vec3 color;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    float value = texture(fontAtlas, fragTexCoord).r;
    outColor = vec4(pc.color, value);
}