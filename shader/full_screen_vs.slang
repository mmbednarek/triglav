static const float2 screenVertices[4] =
{
  float2(-1, -1),
  float2(-1, 1),
  float2(1, 1),
  float2(1, -1),
};

struct FullScreenOut
{
  float4 position: SV_Position;

  [[vk::location(0)]]
  float2 screenCoord: Position0;
};

FullScreenOut vs_main(uint vertexID: SV_VertexID)
{
  FullScreenOut out;
  out.position = float4(screenVertices[vertexID], 0, 1);
  out.screenCoord = 0.5 * (screenVertices[vertexID] + float2(1, 1));
  return out;
}
