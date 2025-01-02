[[vk::binding(0, 0)]]
uniform RWStructuredBuffer<int> InOutBuffer;

[numthreads(1, 1, 1)]
void cs_main()
{
    InOutBuffer[0] += 1;
}
