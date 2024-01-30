#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outAmbient;

layout(binding = 0) uniform sampler2D texPosition;
layout(binding = 1) uniform sampler2D texNormal;
layout(binding = 2) uniform sampler2D texNoise;

layout(binding = 3) uniform AmbientOcclusionUBO {
    mat4 cameraProjection;
    vec3 samplesSSAO[64];
} ubo;

const float radius = 1.2;

void main() {
    vec3 normal = texture(texNormal, fragTexCoord).xyz;
    if (normal == vec3(0.0, 0.0, 0.0)) {
        outAmbient = vec4(1);
        return;
    }
    normal = normalize(normal);

    vec3 position = texture(texPosition, fragTexCoord).xyz;

    vec4 randomPixel = texture(texNoise, fragTexCoord * vec2(1.0, 0.6));
    vec3 randomVector = normalize(randomPixel.xyz * 2 - 1);
    vec3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 ssaoTangentSpace = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < 64; ++i) {
        vec3 s = ssaoTangentSpace * (ubo.samplesSSAO[i]);
        s = position + s * radius;

        vec4 offset = vec4(s, 1.0);
        offset = ubo.cameraProjection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        vec3 occluderPos = texture(texPosition, offset.xy).xyz;
        float rangeCheck = smoothstep(0.0, 1.0, radius / length(position - occluderPos));

        occlusion += (occluderPos.z >= s.z + 0.1 ? rangeCheck : 0.0);
    }

    outAmbient = vec4(1.0 - (occlusion / 64.0));
}