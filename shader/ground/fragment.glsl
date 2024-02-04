#version 450

layout(location = 0) in vec4 fragWorldPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragViewPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

void main() {
    int x = int(floor(0.2 * fragWorldPosition.x));
    int y = int(floor(0.2 * fragWorldPosition.y));

    float modX = mod(fragWorldPosition.x, 5.0);
    float modY = mod(fragWorldPosition.y, 5.0);

    float metallic = 0.0;

    if (modX < 0.05 || modY < 0.05 || modX > 4.95 || modY > 4.95) {
        metallic = 0.4;
        outColor = vec4(0.5, 0.5, 0.5, 0.2);
    } else if ((x+y) % 2 == 0) {
        outColor = vec4(0.9, 0.9, 0.9, 1.0);
    } else {
        outColor = vec4(0.7, 0.7, 0.7, 1.0);
    }

    outPosition = vec4(fragViewPosition.xyz, metallic);
    outNormal = vec4(normalize(fragNormal), 1.0);
}