struct FSInput
{
  [[vk::location(0)]]
  float2 screenCoord: Position0;
};

[[vk::binding(0, 0)]]
Sampler2D<float> DepthTex;

static const float Near = 0.1;
static const float Far = 100.0;

float to_linear_depth(const float depth)
{
   return (2.0f * Near) / (Far + Near - depth * (Far - Near));
}

float4 fs_main(FSInput input) : SV_Target0
{
    float pixel = to_linear_depth(DepthTex.Sample(input.screenCoord));
    return float4(pixel, pixel, pixel, 1);
}
