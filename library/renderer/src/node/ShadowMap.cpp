#include "ShadowMap.hpp"

#include "triglav/graphics_api/PipelineBuilder.hpp"

#include <memory>

namespace triglav::renderer::node {

using namespace name_literals;
using graphics_api::AttachmentAttribute;

constexpr auto g_shadowMapResolution = graphics_api::Resolution{4096, 4096};
constexpr auto g_shadowMapFormat = GAPI_FORMAT(D, Float32);

class ShadowMapResources : public render_core::NodeFrameResources
{
 public:
   using Self = ShadowMapResources;

   ShadowMapResources(graphics_api::Device& device, resource::ResourceManager& resourceManager, graphics_api::Pipeline& pipeline,
                      Scene& scene) :
       m_device(device),
       m_resourceManager(resourceManager),
       m_pipeline(pipeline),
       m_scene(scene),
       TG_CONNECT(scene, OnObjectAddedToScene, on_object_added_to_scene),
       TG_CONNECT(scene, OnViewportChange, on_viewport_change),
       TG_CONNECT(scene, OnShadowMapChanged, on_shadow_map_changed)
   {
   }

   void on_object_added_to_scene(const SceneObject& object)
   {
      const auto& model = m_resourceManager.get<ResourceType::Model>(object.model);

      graphics_api::UniformBuffer<render_objects::ShadowMapUBO> shadowMapUbo1(m_device);
      graphics_api::UniformBuffer<render_objects::ShadowMapUBO> shadowMapUbo2(m_device);
      graphics_api::UniformBuffer<render_objects::ShadowMapUBO> shadowMapUbo3(m_device);
      m_models.emplace_back(
         render_objects::ModelShaderMapProperties{object.model,
                                                  model.boundingBox,
                                                  object.model_matrix(),
                                                  {std::move(shadowMapUbo1), std::move(shadowMapUbo2), std::move(shadowMapUbo3)}});
   }

   void on_viewport_change(const graphics_api::Resolution& /*resolution*/)
   {
      for (u32 smIndex = 0; smIndex < m_scene.directional_shadow_map_count(); ++smIndex) {
         const auto camMatShadow = m_scene.shadow_map_camera(smIndex).view_projection_matrix();
         for (const auto& obj : m_models) {
            obj.ubos[smIndex]->mvp = camMatShadow * obj.modelMat;
         }
      }
   }

   void on_shadow_map_changed(const u32 index, const OrthoCamera& cam)
   {
      const auto camMatShadow = cam.view_projection_matrix();
      for (const auto& obj : m_models) {
         obj.ubos[index]->mvp = camMatShadow * obj.modelMat;
      }
   }

   void draw_model(graphics_api::CommandList& cmdList, const u32 smIndex, const render_objects::ModelShaderMapProperties& instancedModel)
   {
      const auto& model = m_resourceManager.get<ResourceType::Model>(instancedModel.modelName);

      cmdList.bind_vertex_array(model.mesh.vertices);
      cmdList.bind_index_array(model.mesh.indices);

      const auto firstOffset = model.range[0].offset;
      size_t size{};
      for (const auto& range : model.range) {
         size += range.size;
      }

      cmdList.bind_uniform_buffer(0, instancedModel.ubos[smIndex]);
      cmdList.draw_indexed_primitives(static_cast<int>(size), static_cast<int>(firstOffset), 0);
   }

   void draw_scene_models(graphics_api::CommandList& cmdList, const u32 smIndex)
   {
      cmdList.bind_pipeline(m_pipeline);
      for (const auto& obj : m_models) {
         if (not m_scene.shadow_map_camera(smIndex).is_bounding_box_visible(obj.boundingBox, obj.modelMat))
            continue;

         this->draw_model(cmdList, smIndex, obj);
      }
   }

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   graphics_api::Pipeline& m_pipeline;
   Scene& m_scene;
   std::vector<render_objects::ModelShaderMapProperties> m_models;

   TG_SINK(Scene, OnObjectAddedToScene);
   TG_SINK(Scene, OnViewportChange);
   TG_SINK(Scene, OnShadowMapChanged);
};

ShadowMap::ShadowMap(graphics_api::Device& device, resource::ResourceManager& resourceManager, Scene& scene) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_depthRenderTarget(
       GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                     .attachment("sm"_name, AttachmentAttribute::Depth | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage,
                                 g_shadowMapFormat)
                     .build())),
    m_pipeline(GAPI_CHECK(graphics_api::GraphicsPipelineBuilder(device, m_depthRenderTarget)
                             .fragment_shader(resourceManager.get("shadow_map.fshader"_rc))
                             .vertex_shader(resourceManager.get("shadow_map.vshader"_rc))
                             .begin_vertex_layout<geometry::Vertex>()
                             .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                             .end_vertex_layout()
                             .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                             .enable_depth_test(true)
                             .use_push_descriptors(true)
                             .culling(graphics_api::Culling::None)
                             .build())),
    m_scene(scene)
{
}

std::unique_ptr<render_core::NodeFrameResources> ShadowMap::create_node_resources()
{
   auto result = std::make_unique<ShadowMapResources>(m_device, m_resourceManager, m_pipeline, m_scene);
   result->add_render_target_with_resolution("sm1"_name, m_depthRenderTarget, g_shadowMapResolution);
   result->add_render_target_with_resolution("sm2"_name, m_depthRenderTarget, g_shadowMapResolution);
   result->add_render_target_with_resolution("sm3"_name, m_depthRenderTarget, g_shadowMapResolution);
   return result;
}

graphics_api::WorkTypeFlags ShadowMap::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void ShadowMap::record_commands(render_core::FrameResources& /*frameResources*/, render_core::NodeFrameResources& resources,
                                graphics_api::CommandList& cmdList)
{
   std::array clearValues{
      graphics_api::ClearValue{graphics_api::DepthStenctilValue{1.0f, 0}},
   };

   auto& smResources = dynamic_cast<ShadowMapResources&>(resources);

   cmdList.begin_render_pass(resources.framebuffer("sm1"_name), clearValues);
   smResources.draw_scene_models(cmdList, 0);
   cmdList.end_render_pass();

   cmdList.begin_render_pass(resources.framebuffer("sm2"_name), clearValues);
   smResources.draw_scene_models(cmdList, 1);
   cmdList.end_render_pass();

   cmdList.begin_render_pass(resources.framebuffer("sm3"_name), clearValues);
   smResources.draw_scene_models(cmdList, 2);
   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
