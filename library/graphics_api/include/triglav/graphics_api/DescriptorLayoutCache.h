#pragma once

#include "GraphicsApi.hpp"
#include "DescriptorPool.h"

#include <span>
#include <map>

namespace triglav::graphics_api {

class DescriptorLayoutCache
{
public:
   using Hash = u64;
   
   VkDescriptorSetLayout find_layout(std::span<DescriptorBinding> bindings);

private:
   VkDevice m_device;
   VkDescriptorSetLayout construct_layout(Hash hash, std::span<DescriptorBinding> bindings);
   std::map<Hash, vulkan::DescriptorSetLayout> m_pools;
   
};

}
