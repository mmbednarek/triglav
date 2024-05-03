#include "ShadowMap.h"

#include "triglav/graphics_api/DescriptorWriter.h"
#include "triglav/graphics_api/PipelineBuilder.h"

#include <memory>

namespace triglav::renderer::node {

using namespace name_literals;
using graphics_api::AttachmentAttribute;

constexpr auto g_shadowMapResolution = graphics_api::Resolution{4096, 4096};
constexpr auto g_shadowMapFormat     = GAPI_FORMAT(D, Float32);

class ShadowMapResources : public render_core::NodeFrameResources
{
 public:
   ShadowMapResources(graphics_api::Device &device, resource::ResourceManager &resourceManager,
                      graphics_api::Pipeline &pipeline, Scene &scene) :
       m_device(device),
       m_resourceManager(resourceManager),
       m_pipeline(pipeline),
       m_scene(scene),
       m_onAddedObjectSink(
               scene.OnObjectAddedToScene.connect<&ShadowMapResources::on_object_added_to_scene>(this)),
       m_onViewportChangeSink(scene.OnViewportChange.connect<&ShadowMapResources::on_viewport_change>(this))
   {
   }

   void on_object_added_to_scene(const SceneObject &object)
   {
      const auto &model = m_resourceManager.get<ResourceType::Model>(object.model);

      graphics_api::UniformBuffer<render_core::ShadowMapUBO> shadowMapUbo(m_device);
      m_models.emplace_back(render_core::ModelShaderMapProperties{
              object.model, model.boundingBox, object.model_matrix(), std::move(shadowMapUbo)});
   }

   void on_viewport_change(const graphics_api::Resolution & /*resolution*/)
   {
      const auto camMatShadow = m_scene.shadow_map_camera().view_projection_matrix();
      for (const auto &obj : m_models) {
         obj.ubo->mvp = camMatShadow * obj.modelMat;
      }
   }

   void draw_model(graphics_api::CommandList &cmdList,
                   const render_core::ModelShaderMapProperties &instancedModel)
   {
      const auto &model = m_resourceManager.get<ResourceType::Model>(instancedModel.modelName);

      cmdList.bind_vertex_array(model.mesh.vertices);
      cmdList.bind_index_array(model.mesh.indices);

      const auto firstOffset = model.range[0].offset;
      size_t size{};
      for (const auto &range : model.range) {
         size += range.size;
      }

      graphics_api::DescriptorWriter smDescWriter(m_device);
      smDescWriter.set_uniform_buffer(0, instancedModel.ubo);
      cmdList.push_descriptors(0, smDescWriter);

      cmdList.draw_indexed_primitives(size, firstOffset, 0);
   }

   void draw_scene_models(graphics_api::CommandList &cmdList)
   {
      cmdList.bind_pipeline(m_pipeline);

      for (const auto &obj : m_models) {
         if (not m_scene.shadow_map_camera().is_bouding_box_visible(obj.boundingBox, obj.modelMat))
            continue;

         this->draw_model(cmdList, obj);
      }
   }

 private:
   graphics_api::Device &m_device;
   resource::ResourceManager &m_resourceManager;
   graphics_api::Pipeline &m_pipeline;
   Scene &m_scene;
   std::vector<render_core::ModelShaderMapProperties> m_models;
   Scene::OnObjectAddedToSceneDel::Sink<ShadowMapResources> m_onAddedObjectSink;
   Scene::OnViewportChangeDel::Sink<ShadowMapResources> m_onViewportChangeSink;
};

ShadowMap::ShadowMap(graphics_api::Device &device, resource::ResourceManager &resourceManager, Scene &scene) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_depthRenderTarget(
            GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                               .attachment("sm"_name_id,
                                           AttachmentAttribute::Depth | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           g_shadowMapFormat)
                               .build())),
    m_pipeline(GAPI_CHECK(
            graphics_api::PipelineBuilder(device, m_depthRenderTarget)
                    .fragment_shader(
                            resourceManager.get<ResourceType::FragmentShader>("shadow_map.fshader"_name))
                    .vertex_shader(resourceManager.get<ResourceType::VertexShader>("shadow_map.vshader"_name))
                    .begin_vertex_layout<geometry::Vertex>()
                    .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                    .end_vertex_layout()
                    .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                        graphics_api::PipelineStage::VertexShader)
                    .enable_depth_test(true)
                    .use_push_descriptors(true)
                    .build())),
    m_scene(scene)
{
}

std::unique_ptr<render_core::NodeFrameResources> ShadowMap::create_node_resources()
{
   auto result = std::make_unique<ShadowMapResources>(m_device, m_resourceManager, m_pipeline, m_scene);
   result->add_render_target_with_resolution("sm"_name_id, m_depthRenderTarget, g_shadowMapResolution);
   return result;
}

graphics_api::WorkTypeFlags ShadowMap::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void ShadowMap::record_commands(render_core::FrameResources &frameResources,
                                render_core::NodeFrameResources &resources,
                                graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 1> clearValues{
           graphics_api::ClearValue{graphics_api::DepthStenctilValue{1.0f, 0}},
   };

   cmdList.begin_render_pass(resources.framebuffer("sm"_name_id), clearValues);

   auto &smResources = dynamic_cast<ShadowMapResources &>(resources);
   smResources.draw_scene_models(cmdList);

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
