implementing material;

namespace material
{
    [[vk::binding(2)]]
    public uniform Sampler2D<float4> MaterialTextures[];
}
