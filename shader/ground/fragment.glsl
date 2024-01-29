#version 450

layout(location = 0) in vec4 fragWorldPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragViewPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

void main() {
    int x = int(floor(0.1 * fragWorldPosition.x));
    int y = int(floor(0.1 * fragWorldPosition.y));

    float modX = mod(fragWorldPosition.x, 10.0);
    float modY = mod(fragWorldPosition.y, 10.0);

    if (modX < 0.1 || modY < 0.1 || modX > 9.9 || modY > 9.9) {
        outColor = vec4(0.5, 0.5, 0.5, 1.0);
    } else if ((x+y) % 2 == 0) {
        outColor = vec4(0.9, 0.9, 0.9, 1.0);
    } else {
        outColor = vec4(0.7, 0.7, 0.7, 1.0);
    }

    outPosition = vec4(fragViewPosition.xyz, 0.0);
    outNormal = vec4(normalize(fragNormal), 1.0);
}