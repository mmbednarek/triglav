#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D roughnessSampler;
layout(binding = 4) uniform sampler2D metallicSampler;

void main() {
    outColor = vec4(texture(texSampler, fragTexCoord).rgb, texture(roughnessSampler, fragTexCoord).r);
    outPosition = vec4(fragPosition, texture(metallicSampler, fragTexCoord).r);

    const mat3 tangentSpaceMat = mat3(fragTangent, fragBitangent, fragNormal);
    vec3 normalSample = 2 * texture(normalSampler, fragTexCoord).rgb - 1.0;
    outNormal = vec4(normalize(tangentSpaceMat * normalSample), 1.0);
}
