implementing material;

namespace material
{
    public struct VSInput
    {
        [[vk::location(0)]]
        public float3 position : POSITION0;
        [[vk::location(1)]]
        public float2 texCoord : TEXCOORD0;
        [[vk::location(2)]]
        public float3 normal : NORMAL0;
        [[vk::location(3)]]
        public float4 tangent : TANGENT0;
    };

    public struct FSInput
    {
        public float4 position : SV_POSITION;

        [[vk::location(0)]]
        public float3 viewSpacePosition : POSITION0;
        [[vk::location(1)]]
        public float2 texCoord : TEXCOORD0;
        [[vk::location(2)]]
        public float3 normal : NORMAL;
        [[vk::location(3)]]
        public float3 tangent : TANGENT0;
        [[vk::location(4)]]
        public float3 bitangent : TANGENT1;

        [[vk::location(5)]]
        public nointerpolation uint materialID;
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

