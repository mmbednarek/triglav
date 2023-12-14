#pragma once

#include "vulkan/ObjectWrapper.hpp"

namespace graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(Sampler, Device)

class Sampler
{
public:
   explicit Sampler(vulkan::Sampler sampler);

   [[nodiscard]] VkSampler vulkan_sampler() const;
private:
   vulkan::Sampler m_sampler;
};

}