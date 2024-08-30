#version 450

layout(location = 0) in vec3 fragWorldPosition;
layout(location = 1) in vec3 fragViewPosition;
layout(location = 2) in vec3 fragWorldNormal;
layout(location = 3) in vec3 fragViewNormal;

layout(binding = 1) uniform sampler2D texPrevColor;
layout(binding = 2) uniform sampler2D texPrevDepth;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);

layout(binding = 3) uniform WorldProperties
{
    mat4 prevViewProj;
    vec3 viewPos;
} wp;

bool bound_check(vec4 shadowCoord)
{
    return shadowCoord.z > -0.99 && shadowCoord.z < 0.99 &&
    shadowCoord.y > 0.01 && shadowCoord.y < 0.99 &&
    shadowCoord.x > 0.01 && shadowCoord.x < 0.99;
}

void main() {
    outPosition = vec4(fragViewPosition, 0.0);
    outNormal = vec4(normalize(fragViewNormal), 1.0);

    vec3 pos = fragWorldPosition;

    vec3 normal = normalize(fragWorldNormal);
    vec3 viewDir = normalize(wp.viewPos - fragWorldPosition);

    vec3 traceDir = reflect(-viewDir, normal);

    bool found = false;
    vec2 sampleScreenPos = vec2(0, 0);
    float traceDistance = 0.0;
    float traceInc = 0.02;

    for (int i = 0; i < 600; ++i) {
        traceDistance += traceInc;
        vec3 tracePos = pos + traceDir * traceDistance;

        vec4 screenPos = biasMat * wp.prevViewProj * vec4(tracePos, 1);
        screenPos /= screenPos.w;

        if (!bound_check(screenPos)) {
            break;
        }

        float depthValue = texture(texPrevDepth, screenPos.xy).r;
        if (depthValue < screenPos.z) {
            found = true;
            sampleScreenPos = screenPos.xy;
            break;
        }
    }

    if (found) {
        outColor = texture(texPrevColor, sampleScreenPos.xy);
    } else {
        outColor = vec4(0.5, 0.5, 0.5, 1.0);
    }

}
