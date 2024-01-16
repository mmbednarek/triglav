#version 450

layout(location=0) in vec2 fragPosition;

layout(location=0) out vec4 outColor;

layout(binding=1) uniform FragUBO {
    vec4 borderRadius;
    vec4 backgroundColor;
    vec2 rectSize;
} ubo;

void main() {
    const vec2 topLeft = vec2(ubo.borderRadius.x, ubo.borderRadius.x);
    const vec2 topRight = vec2(ubo.rectSize.x - ubo.borderRadius.y, ubo.borderRadius.y);
    const vec2 bottomLeft = vec2(ubo.borderRadius.z, ubo.rectSize.y - ubo.borderRadius.z);
    const vec2 bottomRight = vec2(ubo.rectSize.x - ubo.borderRadius.w, ubo.rectSize.y - ubo.borderRadius.w);

    if (fragPosition.x < topLeft.x && fragPosition.y < topLeft.y) {
        float distance = distance(fragPosition, topLeft);
        if (distance < ubo.borderRadius.x) {
            float alpha = 1.0 - pow(distance / ubo.borderRadius.x, 50.0);
            outColor = vec4(ubo.backgroundColor.rgb, ubo.backgroundColor.a * alpha);
        } else {
            outColor = vec4(0.0, 0.0, 0.0, 0.0);
        }
        return;
    }

    if (fragPosition.x > topRight.x && fragPosition.y < topRight.y) {
        float distance = distance(fragPosition, topRight);
        if (distance < ubo.borderRadius.y) {
            float alpha = 1.0 - pow(distance / ubo.borderRadius.y, 50.0);
            outColor = vec4(ubo.backgroundColor.rgb, ubo.backgroundColor.a * alpha);
        } else {
            outColor = vec4(0.0, 0.0, 0.0, 0.0);
        }
        return;
    }

    if (fragPosition.x < bottomLeft.x && fragPosition.y > bottomLeft.y) {
        float distance = distance(fragPosition, bottomLeft);
        if (distance < ubo.borderRadius.z) {
            float alpha = 1.0 - pow(distance / ubo.borderRadius.z, 50.0);
            outColor = vec4(ubo.backgroundColor.rgb, ubo.backgroundColor.a * alpha);
        } else {
            outColor = vec4(0.0, 0.0, 0.0, 0.0);
        }
        return;
    }

    if (fragPosition.x > bottomRight.x && fragPosition.y > bottomRight.y) {
        float distance = distance(fragPosition, bottomRight);
        if (distance < ubo.borderRadius.w) {
            float alpha = 1.0 - pow(distance / ubo.borderRadius.w, 50.0);
            outColor = vec4(ubo.backgroundColor.rgb, ubo.backgroundColor.a * alpha);
        } else {
            outColor = vec4(0.0, 0.0, 0.0, 0.0);
        }
        return;
    }

    outColor = ubo.backgroundColor;
}