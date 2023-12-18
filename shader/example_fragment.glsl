#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosition;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

const vec3 lightPosition = vec3(0, 0, -20);
const vec3 black = vec3(0, 0, 0);
const vec3 white = vec3(1, 1, 1);
const float ambientStrength = 0.1;

void main() {
    vec4 texPixel = texture(texSampler, fragTexCoord);

    vec3 ambient = ambientStrength * white;
    vec3 lightDir = normalize(lightPosition - fragPosition);
//    vec3 lightDir = vec3(1.0, 0.0, 0.0);
    float diff = max(dot(normalize(fragNormal), normalize(lightDir)), 0.0);
    vec3 diffuse = diff * white;

    vec3 result = (ambient + diffuse) * texPixel.rgb;
    outColor = vec4(result, 1);
}
