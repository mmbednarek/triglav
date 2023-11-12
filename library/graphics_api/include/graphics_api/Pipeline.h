#pragma once

#include "GraphicsApi.hpp"
#include "vulkan/ObjectWrapper.hpp"

#include <variant>

namespace graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(PipelineLayout, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorPool, Device)
DECLARE_VLK_WRAPPED_CHILD_OBJECT(DescriptorSetLayout, Device)

class Buffer;
class Texture;

namespace vulkan {
using Pipeline = WrappedObject<VkPipeline, vkCreateGraphicsPipelines, vkDestroyPipeline, VkDevice>;
}

struct DescriptorWrite
{
   DescriptorType type{};
   int binding{};
   std::variant<Buffer *, Texture *> data;
};

class Pipeline
{
 public:
   Pipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline, vulkan::DescriptorPool descriptorPool,
            vulkan::DescriptorSetLayout descriptorSetLayout, std::vector<VkDescriptorSet> descriptorSets, VkSampler sampler);
   ~Pipeline();

   Pipeline(const Pipeline& other) = delete;
   Pipeline& operator=(const Pipeline& other) = delete;
   Pipeline(Pipeline&& other) noexcept;
   Pipeline& operator=(Pipeline&& other) noexcept;

   [[nodiscard]] VkPipeline vulkan_pipeline() const;
   [[nodiscard]] const vulkan::PipelineLayout &layout() const;
   [[nodiscard]] const std::vector<VkDescriptorSet> &descriptor_sets() const;
   [[nodiscard]] size_t descriptor_set_count() const;

   void update_descriptors(size_t descriptorId, std::span<DescriptorWrite> descriptors);

 private:
   vulkan::PipelineLayout m_layout;
   vulkan::Pipeline m_pipeline;
   vulkan::DescriptorPool m_descriptorPool;
   vulkan::DescriptorSetLayout m_descriptorSetLayout;
   VkSampler m_sampler;
   std::vector<VkDescriptorSet> m_descriptorSets;
};

}// namespace graphics_api
