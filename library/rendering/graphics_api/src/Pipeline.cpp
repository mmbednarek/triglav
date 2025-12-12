#include "Pipeline.hpp"

#include "vulkan/Util.hpp"

namespace triglav::graphics_api {

Pipeline::Pipeline(vulkan::PipelineLayout layout, vulkan::Pipeline pipeline, vulkan::DescriptorSetLayout descriptor_set_layout,
                   PipelineType pipeline_type) :
    m_layout(std::move(layout)),
    m_pipeline(std::move(pipeline)),
    m_descriptor_set_layout(std::move(descriptor_set_layout)),
    m_pipeline_type(pipeline_type)
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
   return m_pipeline_type;
}

VkDescriptorSetLayout Pipeline::vulkan_descriptor_set_layout() const
{
   return *m_descriptor_set_layout;
}

}// namespace triglav::graphics_api
