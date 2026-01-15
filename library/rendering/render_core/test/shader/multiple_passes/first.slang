struct FSInput
{
  [[vk::location(0)]]
  float2 screenCoord: Position0;
};

float4 fs_main(FSInput input) : SV_Target0
{
    return float4(input.screenCoord, 1, 1);
}

