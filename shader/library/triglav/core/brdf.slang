implementing core;

namespace triglav
{
    float distribution_ggx(float NdotH, float roughness)
    {
        float alpha = roughness * roughness;
        float alphaSq = alpha * alpha;
        float denominator = NdotH * NdotH * (alphaSq - 1.0) + 1.0;
        denominator = PI * denominator * denominator;
        return alphaSq / max(denominator, 0.000001);
    }

    float smith_formula(float NdotV, float NdotL, float roughness)
    {
        float r = roughness + 1.0;
        float k = (r*r) / 8.0;
        float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
        float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
        return ggx1 * ggx2;
    }

    float3 fresnel_schlick(float HdotV, float3 baseReflectivity)
    {
        return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - HdotV, 5.0);
    }

    public struct ShadingInfo
    {
        public float3 normal;
        public float3 viewDir;
        public float3 position;
        public float3 baseReflectivity;
        public float3 albedo;
        public float roughness;
        public float metallic;

        public float3 calculate_directional_light(float3 lightDir)
        {
            float3 halfpoint = normalize(this.viewDir + lightDir);
            float attenuation = 4.0;
            float3 radience = float3(1, 1, 1) * attenuation;

            float NdotV = max(dot(this.normal, this.viewDir), 0.00000001);
            float NdotL = max(dot(this.normal, lightDir), 0.00000001);
            float HdotV = max(dot(halfpoint, this.viewDir), 0);
            float NdotH = max(dot(this.normal, halfpoint), 0);

            float dist = distribution_ggx(NdotH, this.roughness);
            float geo = smith_formula(NdotV, NdotL, this.roughness);
            float3 fresnel = fresnel_schlick(HdotV, this.baseReflectivity);

            float3 specular = dist * geo * fresnel;
            specular /= 4.0 * NdotV * NdotL;

            float3 kD = float3(1.0) - fresnel;
            kD *= 1.0 - this.metallic;

            return (kD * this.albedo / PI + specular) * radience * NdotL;
        }

        public float3 calculate_point_light(float3 lightPosition, float3 lightColor, float range)
        {
            float3 posDiff = lightPosition - this.position;

            float lightDist = length(posDiff);
            if (lightDist > range) {
                return float3(0);
            }

            float3 lightDir = normalize(posDiff);
            return pow(1 - lightDist/range, 2) * lightColor * this.calculate_directional_light(lightDir);
        }
    };
}