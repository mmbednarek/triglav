#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normal;
    vec3 viewPosition;
} ubo;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;
layout(location = 5) out vec3 fragWorldPosition;
layout(location = 6) out vec3 fragWorldNormal;
layout(location = 7) out vec3 fragWorldTangent;
layout(location = 8) out vec3 fragWorldBitangent;

void main() {
    vec4 viewSpace = ubo.view * ubo.model * vec4(inPosition, 1.0);
    const mat3 normMat = mat3(ubo.normal);

    fragTexCoord = inTexCoord;
    mat3 viewNormalMat = mat3(ubo.view) * normMat;
    fragNormal = viewNormalMat * inNormal;
    fragTangent = viewNormalMat * inTangent;
    fragBitangent = viewNormalMat * inBitangent;
    fragPosition = viewSpace.xyz;
    fragWorldPosition = (ubo.model * vec4(inPosition, 1.0)).xyz;

    const mat3 tangentSpaceMat = inverse(mat3(inTangent, inBitangent, inNormal));

    fragWorldTangent = tangentSpaceMat[0];
    fragWorldBitangent = tangentSpaceMat[1];
    fragWorldNormal = tangentSpaceMat[2];

    gl_Position = ubo.proj * viewSpace;
}