#include "Geometry.h"

#include "triglav/graphics_api/Framebuffer.h"

namespace triglav::renderer::node {

using namespace name_literals;
using graphics_api::AttachmentAttribute;

Geometry::Geometry(graphics_api::Device &device, resource::ResourceManager &resourceManager) :
    m_renderTarget(
            GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                               .attachment("albedo"_name_id,
                                           AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(RGBA, Float16))
                               .attachment("position"_name_id,
                                           AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(RGBA, Float16))
                               .attachment("normal"_name_id,
                                           AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(RGBA, Float16))
                               .attachment("depth"_name_id,
                                           AttachmentAttribute::Depth | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(D, UNorm16))
                               .build())),
    m_skybox(device, resourceManager, m_renderTarget),
    m_groundRenderer(device, m_renderTarget, resourceManager),
    m_modelRenderer(device, m_renderTarget, resourceManager),
    m_debugLinesRenderer(device, m_renderTarget, resourceManager),
    m_scene(m_modelRenderer, m_debugLinesRenderer, resourceManager),
    m_timestampArray(GAPI_CHECK(device.create_timestamp_array(2)))
{
}

graphics_api::WorkTypeFlags Geometry::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void Geometry::record_commands(render_core::FrameResources &frameResources,
                               render_core::NodeFrameResources &resources, graphics_api::CommandList &cmdList)
{
   cmdList.reset_timestamp_array(m_timestampArray, 0, 2);
   cmdList.write_timestamp(graphics_api::PipelineStage::Entrypoint, m_timestampArray, 0);

   std::array<graphics_api::ClearValue, 4> clearValues{
           graphics_api::ColorPalette::Black,
           graphics_api::ColorPalette::Black,
           graphics_api::ColorPalette::Black,
           graphics_api::DepthStenctilValue{1.0f, 0},
   };

   auto &framebuffer = resources.framebuffer("gbuffer"_name_id);
   cmdList.begin_render_pass(framebuffer, clearValues);

   m_skybox.on_render(cmdList, m_scene.yaw(), m_scene.pitch(),
                      static_cast<float>(framebuffer.resolution().width),
                      static_cast<float>(framebuffer.resolution().height));

   m_groundRenderer.draw(cmdList, m_scene.camera());

   m_modelRenderer.begin_render(cmdList);
   m_scene.render(cmdList);

   if (frameResources.has_flag("debug_lines"_name_id)) {
      m_debugLinesRenderer.begin_render(cmdList);
      m_scene.render_debug_lines(cmdList);
   }

   cmdList.end_render_pass();

   cmdList.write_timestamp(graphics_api::PipelineStage::End, m_timestampArray, 1);
}

std::unique_ptr<render_core::NodeFrameResources> Geometry::create_node_resources()
{
   auto result = IRenderNode::create_node_resources();
   result->add_render_target("gbuffer"_name_id, m_renderTarget);
   return result;
}

float Geometry::gpu_time() const
{
   return m_timestampArray.get_difference(0, 1);
}

Scene &Geometry::scene()
{
   return m_scene;
}

}// namespace triglav::renderer::node
