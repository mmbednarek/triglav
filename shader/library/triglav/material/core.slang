implementing material;

namespace triglav::material
{
    public struct VSInput
    {
        public float3 position : POSITION0;
        public float3 normal : NORMAL0;
        public float2 texCoord : TEXCOORD0;
#if MT_USE_NORMAL_MAP
        public float4 tangent : TANGENT0;
#endif
#if MT_USE_BONES
        public int4 bone_ids : BLENDINDICES0;
        public float4 bone_weights : BLENDWEIGHT0;
#endif
    };

    public struct FSInput
    {
        public float4 position : SV_POSITION;

        [[vk::location(0)]]
        public float3 viewSpacePosition : POSITION0;
        [[vk::location(1)]]
        public float3 normal : NORMAL;
        [[vk::location(2)]]
        public float2 texCoord : TEXCOORD0;
        [[vk::location(3)]]
        public nointerpolation uint materialID;

#if MT_USE_NORMAL_MAP
        [[vk::location(4)]]
        public float3 tangent : TANGENT0;
        [[vk::location(5)]]
        public float3 bitangent : TANGENT1;
#endif
    };

    public typedef FSInput VSOutput;

    public struct FSOutput
    {
        [[vk::location(0)]]
        public float4 color : SV_TARGET0;
        [[vk::location(1)]]
        public float4 position : SV_TARGET1;
        [[vk::location(2)]]
        public float4 normal : SV_TARGET2;
    };
}

