#include "Pipeline.hpp"

#include "vulkan/Util.hpp"

namespace triglav::graphics_api {

Pipeline::Pipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline, vulkan::DescriptorSetLayout descriptorSetLayout,
                   PipelineType pipelineType) :
    m_layout(std::move(layout)),
    m_pipeline(std::move(pipeline)),
    m_descriptorSetLayout(std::move(descriptorSetLayout)),
    m_pipelineType(pipelineType)
{
}

VkPipeline Pipeline::vulkan_pipeline() const
{
   return *m_pipeline;
}

const vulkan::PipelineLayout& Pipeline::layout() const
{
   return m_layout;
}

PipelineType Pipeline::pipeline_type() const
{
   return m_pipelineType;
}

VkDescriptorSetLayout Pipeline::vulkan_descriptor_set_layout() const
{
   return *m_descriptorSetLayout;
}

}// namespace triglav::graphics_api
