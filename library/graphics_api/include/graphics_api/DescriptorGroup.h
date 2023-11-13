#pragma once

#include "GraphicsApi.hpp"

#include <variant>
#include <vector>
#include <vulkan/vulkan.h>

namespace graphics_api {

class Buffer;
class Texture;

struct DescriptorWrite
{
   DescriptorType type{};
   int binding{};
   std::variant<Buffer *, Texture *> data;
};

class DescriptorGroup
{
 public:
   DescriptorGroup(VkDevice device, VkDescriptorPool descriptorPool, VkSampler sampler,
                   VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSet> descriptorSets);
   ~DescriptorGroup();

   DescriptorGroup(const DescriptorGroup &other)            = delete;
   DescriptorGroup &operator=(const DescriptorGroup &other) = delete;
   DescriptorGroup(DescriptorGroup &&other) noexcept;
   DescriptorGroup &operator=(DescriptorGroup &&other) noexcept;

   [[nodiscard]] size_t count() const;
   [[nodiscard]] VkPipelineLayout pipeline_layout() const;
   [[nodiscard]] const VkDescriptorSet& at(size_t index) const;
   void update(size_t descriptorId, std::span<DescriptorWrite> descriptors);

 private:
   VkDevice m_device;
   VkDescriptorPool m_descriptorPool;
   VkPipelineLayout m_pipelineLayout;
   VkSampler m_sampler;
   std::vector<VkDescriptorSet> m_descriptorSets;
};

}// namespace graphics_api
