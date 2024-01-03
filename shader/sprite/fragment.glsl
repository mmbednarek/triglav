#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

float LinearizeDepth(float depth)
{
    float n = 0.1;
    float f = 100.0;
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}

void main() {
    vec4 tex = texture(texSampler, fragTexCoord);
//    float value = LinearizeDepth(tex.r);
//    outColor = vec4(value, value, value, 1.0);
    outColor = tex;
}
