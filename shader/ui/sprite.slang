struct VertexOutput
{
    float4 position: SV_Position;
  
    [[vk::location(0)]]
    float2 texCoord;
    [[vk::location(1)]]
    nointerpolation uint32_t imageID;
};

[[vk::binding(0)]]
cbuffer VertUBO
{
    float2 g_spriteOffset;
    float2 g_spriteSize;
    float2 g_uvOffset;
    float2 g_uvSize;
    float2 g_screenSize;
    uint32_t g_imageID;
};

static const float2 g_vertexCoords[4] =
{
  float2(0, 0),
  float2(0, 1),
  float2(1, 1),
  float2(1, 0),
};

float2 to_screen_coords(float2 v)
{
    return 2 * (v / g_screenSize) - float2(1, 1);
}

VertexOutput vs_main(uint vertexID: SV_VertexID)
{
    const float2 vertex = g_vertexCoords[vertexID];

    VertexOutput output;
    output.position = float4(to_screen_coords(g_spriteOffset + vertex * g_spriteSize), 0, 1);
    output.texCoord = g_uvOffset + vertex * g_uvSize;
    output.imageID = g_imageID;

    return output;
}

[[vk::binding(1)]]
uniform Sampler2D<float4> g_textures[];

typealias FragmentInput = VertexOutput;

float4 fs_main(FragmentInput input) : SV_Target0
{
    return g_textures[input.imageID].Sample(input.texCoord);
}
