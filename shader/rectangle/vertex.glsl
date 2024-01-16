#version 450

layout(location=0) in vec2 inPosition;

layout(location=0) out vec2 fragPosition;

layout(binding=0) uniform VertUBO {
    vec2 position;
    vec2 viewportSize;
} ubo;

void main() {
    fragPosition = inPosition - ubo.position;
    gl_Position = vec4(2 * (inPosition / ubo.viewportSize) - vec2(1, 1), 0, 1);
}