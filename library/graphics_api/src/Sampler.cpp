#include "Sampler.h"

namespace triglav::graphics_api {

Sampler::Sampler(vulkan::Sampler sampler) :
    m_sampler(std::move(sampler))
{
}

VkSampler Sampler::vulkan_sampler() const
{
   return *m_sampler;
}

}// namespace graphics_api