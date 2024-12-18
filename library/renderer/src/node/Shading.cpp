#include "Shading.hpp"
#include "Particles.hpp"
#include "TextureHelper.hpp"

#include "triglav/graphics_api/PipelineBuilder.hpp"

namespace triglav::renderer::node {

using graphics_api::AttachmentAttribute;
using graphics_api::PipelineStage;
using graphics_api::SampleCount;
using graphics_api::TextureBarrierInfo;
using graphics_api::TextureState;

using namespace name_literals;

constexpr geometry::BoundingBox g_particlesBoundingBox{
   {-31.0f, -1.0f, -50.0f},
   {-29.0f, 1.0f, 0.0f},
};

class ShadingResources : public render_core::NodeFrameResources
{
 public:
   explicit ShadingResources(graphics_api::Device& device) :
       m_uniformBuffer(device)
   {
   }

   graphics_api::UniformBuffer<ShadingRenderer::UniformData>& get_ubo()
   {
      return m_uniformBuffer;
   }

 private:
   graphics_api::UniformBuffer<ShadingRenderer::UniformData> m_uniformBuffer;
};

Shading::Shading(graphics_api::Device& device, resource::ResourceManager& resourceManager, Scene& scene) :
    m_device(device),
    m_shadingRenderTarget(GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                                        .attachment("shading"_name,
                                                    AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                       AttachmentAttribute::StoreImage | AttachmentAttribute::Storage,
                                                    GAPI_FORMAT(RGBA, Float16), SampleCount::Single)
                                        .attachment("bloom"_name,
                                                    AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                       AttachmentAttribute::StoreImage,
                                                    GAPI_FORMAT(RGBA, Float16), SampleCount::Single)
                                        .attachment("depth"_name,
                                                    AttachmentAttribute::Depth | AttachmentAttribute::LoadImage |
                                                       AttachmentAttribute::StoreImage | AttachmentAttribute::TransferDst,
                                                    GAPI_FORMAT(D, UNorm16))
                                        .build())),
    m_shadingRenderer(device, m_shadingRenderTarget, resourceManager),
    m_scene(scene),
    m_particlesPipeline(
       GAPI_CHECK(graphics_api::GraphicsPipelineBuilder(device, m_shadingRenderTarget)
                     .fragment_shader(resourceManager.get("particles.fshader"_rc))
                     .vertex_shader(resourceManager.get("particles.vshader"_rc))
                     // Descriptor layout
                     .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                     .descriptor_binding(graphics_api::DescriptorType::StorageBuffer, graphics_api::PipelineStage::VertexShader)
                     .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                     .depth_test_mode(graphics_api::DepthTestMode::ReadOnly)
                     .enable_blending(true)
                     .use_push_descriptors(true)
                     .vertex_topology(graphics_api::VertexTopology::TriangleStrip)
                     .build())),
    m_particlesUBO(device),
    m_particlesTexture(resourceManager.get("particle.tex"_rc)),
    m_timestampArray(GAPI_CHECK(device.create_timestamp_array(2)))
{
}

std::unique_ptr<render_core::NodeFrameResources> Shading::create_node_resources()
{
   auto result = std::make_unique<ShadingResources>(m_device);
   result->add_render_target("shading"_name, m_shadingRenderTarget);
   return result;
}

graphics_api::WorkTypeFlags Shading::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void Shading::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                              graphics_api::CommandList& cmdList)
{
   cmdList.reset_timestamp_array(m_timestampArray, 0, 2);
   cmdList.write_timestamp(PipelineStage::Entrypoint, m_timestampArray, 0);

   auto& gbuffer = frameResources.node("geometry"_name).framebuffer("gbuffer"_name);
   auto& framebuffer = resources.framebuffer("shading"_name);

   copy_texture(cmdList, gbuffer.texture("depth"_name), framebuffer.texture("depth"_name));

   std::array<graphics_api::ClearValue, 2> clearValues{
      graphics_api::ColorPalette::Black,
      graphics_api::ColorPalette::Black,
   };
   cmdList.begin_render_pass(framebuffer, clearValues);

   auto inverseViewMat = glm::inverse(m_scene.camera().view_matrix());
   std::array<glm::mat4, 3> shadowMats{};
   shadowMats[0] = m_scene.shadow_map_camera(0).view_projection_matrix() * inverseViewMat;
   shadowMats[1] = m_scene.shadow_map_camera(1).view_projection_matrix() * inverseViewMat;
   shadowMats[2] = m_scene.shadow_map_camera(2).view_projection_matrix() * inverseViewMat;
   const auto lightPosition = m_scene.camera().view_matrix() * glm::vec4(-30, 0, -5, 1.0);

   auto& shadingResources = dynamic_cast<ShadingResources&>(resources);

   m_shadingRenderer.draw(frameResources, cmdList, glm::vec3(lightPosition), shadowMats, m_scene.camera().view_matrix(),
                          shadingResources.get_ubo());

   if (m_scene.camera().is_bounding_box_visible(g_particlesBoundingBox, glm::mat4{1})) {
      auto& particles = dynamic_cast<ParticlesResources&>(frameResources.node("particles"_name));

      m_particlesUBO->view = m_scene.camera().view_matrix();
      m_particlesUBO->proj = m_scene.camera().projection_matrix();

      cmdList.bind_pipeline(m_particlesPipeline);
      cmdList.bind_uniform_buffer(0, m_particlesUBO);
      cmdList.bind_storage_buffer(1, particles.particles_buffer());
      cmdList.bind_texture(2, m_particlesTexture);
      cmdList.draw_primitives(4, 0, 256, 0);
   }

   cmdList.end_render_pass();

   cmdList.write_timestamp(PipelineStage::End, m_timestampArray, 1);
}

float Shading::gpu_time() const
{
   return m_timestampArray.get_difference(0, 1);
}

}// namespace triglav::renderer::node
