implementing material;

namespace material
{
    [[vk::binding(2)]]
    uniform Sampler2D<float4> MaterialTextures[];
}
