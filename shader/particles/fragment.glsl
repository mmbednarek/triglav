#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float fragAnimation;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBloom;

layout(binding = 2) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, fragTexCoord).rgba;
    outColor.rgb *= vec3(1.0, 0.1, 0.1) * fragAnimation + vec3(1.0, 1.0, 0.1) * (1.0 - fragAnimation);
    outBloom = vec4(0.3, 0.3, 0.3, 0.0) + outColor;
}
