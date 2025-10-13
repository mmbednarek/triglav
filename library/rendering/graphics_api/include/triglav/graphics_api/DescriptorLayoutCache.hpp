#pragma once

#include "DescriptorPool.hpp"
#include "GraphicsApi.hpp"

#include <map>
#include <span>

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

}// namespace triglav::graphics_api
