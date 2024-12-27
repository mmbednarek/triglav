[[vk::binding(0, 0)]]
Sampler2D<float4> InputTex;

struct FSInput
{
  [[vk::location(0)]]
  float2 screenCoord: Position0;
};

float4 fs_main(FSInput input) : SV_Target0
{
  float4 pixel = InputTex.Sample(float2(1, 1) - input.screenCoord);

  return float4(1, pixel.y, pixel.x, 1);
}

