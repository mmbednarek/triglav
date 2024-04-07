#include "DescriptorLayoutCache.h"

#include "vulkan/Util.h"

#include <algorithm>
#include <stdexcept>

namespace triglav::graphics_api {

namespace {

DescriptorLayoutCache::Hash hash_bindings(const std::span<DescriptorBinding> bindings)
{
   DescriptorLayoutCache::Hash result{};
   for (const auto& binding : bindings) {
      result *= 57922373ull;
      result += 26987ull * binding.count + 15559ull * static_cast<u64>(binding.stage) + 39181ull * static_cast<u64>(binding.type);
   }
   return result;
}

}

VkDescriptorSetLayout DescriptorLayoutCache::find_layout(const std::span<DescriptorBinding> bindings)
{
   const auto hash = hash_bindings(bindings);
   auto it = m_pools.find(hash);
   if (it != m_pools.end()) {
      // TODO: Verify no collision happened
      return *it->second;
   }

   return this->construct_layout(hash, bindings);
}

VkDescriptorSetLayout DescriptorLayoutCache::construct_layout(const Hash hash, const std::span<DescriptorBinding> bindings)
{
   std::vector<VkDescriptorSetLayoutBinding> layoutBindings{};
   layoutBindings.resize(bindings.size());

   std::ranges::transform(bindings, layoutBindings.begin(), [](const DescriptorBinding& binding) {
      VkDescriptorSetLayoutBinding outBinding{};
      outBinding.descriptorCount = binding.count;
      outBinding.binding = binding.binding;
      outBinding.descriptorType = vulkan::to_vulkan_descriptor_type(binding.type);
      outBinding.stageFlags = vulkan::to_vulkan_shader_stage_flags(binding.stage);
      return outBinding;
   });

   VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
   descriptorSetLayoutInfo.bindingCount = layoutBindings.size();
   descriptorSetLayoutInfo.pBindings = layoutBindings.data();

   vulkan::DescriptorSetLayout layout{m_device};
   if (const auto res = layout.construct(&descriptorSetLayoutInfo); res != VK_SUCCESS) {
      throw std::runtime_error("error creating layout");
   }
   
   auto [it, success] = m_pools.emplace(hash, std::move(layout));
   assert(success);

   return *it->second;
}

}
