#include "DescriptorLayoutCache.hpp"

#include "vulkan/Util.hpp"

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

}// namespace

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
   std::vector<VkDescriptorSetLayoutBinding> layout_bindings{};
   layout_bindings.resize(bindings.size());

   std::ranges::transform(bindings, layout_bindings.begin(), [](const DescriptorBinding& binding) {
      VkDescriptorSetLayoutBinding out_binding{};
      out_binding.descriptorCount = binding.count;
      out_binding.binding = binding.binding;
      out_binding.descriptorType = vulkan::to_vulkan_descriptor_type(binding.type);
      out_binding.stageFlags = vulkan::to_vulkan_shader_stage_flags(binding.stage);
      return out_binding;
   });

   VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
   descriptor_set_layout_info.bindingCount = static_cast<u32>(layout_bindings.size());
   descriptor_set_layout_info.pBindings = layout_bindings.data();

   vulkan::DescriptorSetLayout layout{m_device};
   if (const auto res = layout.construct(&descriptor_set_layout_info); res != VK_SUCCESS) {
      throw std::runtime_error("error creating layout");
   }

   auto [it, success] = m_pools.emplace(hash, std::move(layout));
   assert(success);

   return *it->second;
}

}// namespace triglav::graphics_api
