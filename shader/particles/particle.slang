module particle;

public struct Particle {
    public float3 position;
    public float3 velocity;
    public float animation;
    public float rotation;
    public float angularVelocity;
    public float scale;
};

public struct VertexOut
{
    public float4 position : SV_Position;

    [[vk::location(0)]]
    public float2 texCoord : TEXCOORD0;

    [[vk::location(1)]]
    public nointerpolation float animation;
};

public typealias FragmentIn = VertexOut;
