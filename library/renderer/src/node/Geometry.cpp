#include "Geometry.h"

#include "triglav/graphics_api/Framebuffer.h"

namespace triglav::renderer::node {

Geometry::Geometry(graphics_api::Device& device, Scene &scene, SkyBox &skybox, graphics_api::Framebuffer &modelFramebuffer,
                   GroundRenderer &groundRenderer, ModelRenderer &modelRenderer) :
    m_scene(scene),
    m_skybox(skybox),
    m_modelFramebuffer(modelFramebuffer),
    m_groundRenderer(groundRenderer),
    m_modelRenderer(modelRenderer),
    m_timestampArray(GAPI_CHECK(device.create_timestamp_array(2)))
{
}

graphics_api::WorkTypeFlags Geometry::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void Geometry::record_commands(render_core::FrameResources &frameResources,
                               graphics_api::CommandList &cmdList)
{
   cmdList.reset_timestamp_array(m_timestampArray, 0, 2);
   cmdList.write_timestamp(graphics_api::PipelineStage::Entrypoint, m_timestampArray, 0);

   std::array<graphics_api::ClearValue, 4> clearValues{
           graphics_api::ColorPalette::Black,
           graphics_api::ColorPalette::Black,
           graphics_api::ColorPalette::Black,
           graphics_api::DepthStenctilValue{1.0f, 0.0f},
   };
   cmdList.begin_render_pass(m_modelFramebuffer, clearValues);

   m_skybox.on_render(cmdList, m_scene.yaw(), m_scene.pitch(),
                      static_cast<float>(m_modelFramebuffer.resolution().width),
                      static_cast<float>(m_modelFramebuffer.resolution().height));

   m_groundRenderer.draw(cmdList, m_scene.camera());

   m_modelRenderer.begin_render(cmdList);
   m_scene.render(cmdList);

   // if (m_showDebugLines) {
   //    m_debugLinesRenderer.begin_render(m_commandList);
   //    m_scene.render_debug_lines();
   // }

   cmdList.end_render_pass();

   cmdList.write_timestamp(graphics_api::PipelineStage::End, m_timestampArray, 1);
}

float Geometry::gpu_time() const
{
   std::array<u64, 2> timestamps{};
   m_timestampArray.get_result(timestamps, 0);

   return static_cast<float>(timestamps[1] - timestamps[0]) / 1000000.0f;
}

}// namespace triglav::renderer::node
