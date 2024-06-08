#include "UserInterface.h"

#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/render_core/GlyphAtlas.h"
#include "triglav/ui_core/Viewport.h"

#include <ranges>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

using triglav::graphics_api::AttachmentAttribute;

namespace triglav::renderer::node {

using namespace name_literals;

struct TextColorConstant
{
   glm::vec3 color;
};

class UserInterfaceResources : public render_core::NodeFrameResources
{
 public:
   struct TextObject
   {
      render_core::TextMetric metric;
      graphics_api::UniformBuffer<triglav::render_core::SpriteUBO> ubo;
      graphics_api::VertexArray<render_core::GlyphVertex> vertices;
      u32 vertexCount;
   };

   UserInterfaceResources(graphics_api::Device& device, resource::ResourceManager& resourceManager, ui_core::Viewport& viewport,
                          RectangleRenderer& rectangleRenderer, graphics_api::Pipeline& textPipeline) :
       m_device(device),
       m_resourceManager(resourceManager),
       m_viewport(viewport),
       m_rectangleRenderer(rectangleRenderer),
       m_textPipeline(textPipeline),
       m_onAddedTextSink(viewport.OnAddedText.connect<&UserInterfaceResources::on_added_text>(this)),
       m_onTextChangeContentSink(viewport.OnTextChangeContent.connect<&UserInterfaceResources::on_text_content_change>(this)),
       m_onAddedRectangleSink(viewport.OnAddedRectangle.connect<&UserInterfaceResources::on_added_rectangle>(this))
   {
   }

   void on_added_text(Name id, const ui_core::Text& textObj)
   {
      graphics_api::UniformBuffer<render_core::SpriteUBO> ubo(m_device);

      auto& atlas = m_resourceManager.get<ResourceType::GlyphAtlas>(textObj.glyphAtlas);

      render_core::TextMetric metric{};
      const auto vertices = atlas.create_glyph_vertices(textObj.content, &metric);
      graphics_api::VertexArray<render_core::GlyphVertex> gpuVertices(m_device, vertices.size());
      gpuVertices.write(vertices.data(), vertices.size());

      m_textResources.emplace(id, TextObject(metric, std::move(ubo), std::move(gpuVertices), vertices.size()));
   }

   void on_text_content_change(Name id, const ui_core::Text& /*content*/)
   {
      m_textChanges.emplace_back(id);
   }

   void on_added_rectangle(Name id, const ui_core::Rectangle& rectObj)
   {
      m_rectangleResources.emplace(id, m_rectangleRenderer.create_rectangle(rectObj.rect));
   }

   void update_text(const Name name)
   {
      auto& textObj = m_viewport.text(name);

      auto& atlas = m_resourceManager.get<ResourceType::GlyphAtlas>(textObj.glyphAtlas);

      auto& textRes = m_textResources.at(name);
      const auto vertices = atlas.create_glyph_vertices(textObj.content, &textRes.metric);
      if (vertices.size() > textRes.vertices.count()) {
         graphics_api::VertexArray<render_core::GlyphVertex> gpuVertices(m_device, vertices.size());
         gpuVertices.write(vertices.data(), vertices.size());
         textRes.vertexCount = vertices.size();
         textRes.vertices = std::move(gpuVertices);
      } else {
         textRes.vertices.write(vertices.data(), vertices.size());
         textRes.vertexCount = vertices.size();
      }
   }

   void process_pending_updates()
   {
      for (const Name name : m_textChanges) {
         this->update_text(name);
      }

      m_textChanges.clear();
   }

   void draw_text(graphics_api::CommandList& cmdList, const Name name, const TextObject& textRes)
   {
      const auto [viewportWidth, viewportHeight] = this->framebuffer("ui"_name).resolution();

      auto& textObj = m_viewport.text(name);

      TextColorConstant constant{textObj.color};
      cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, constant);

      const auto sc =
         glm::scale(glm::mat3(1), glm::vec2(2.0f / static_cast<float>(viewportWidth), 2.0f / static_cast<float>(viewportHeight)));
      textRes.ubo->transform = glm::translate(sc, glm::vec2(textObj.position.x - static_cast<float>(viewportWidth) / 2.0f,
                                                            textObj.position.y - static_cast<float>(viewportHeight) / 2.0f));


      cmdList.bind_vertex_array(textRes.vertices);

      auto& atlas = m_resourceManager.get<ResourceType::GlyphAtlas>(textObj.glyphAtlas);

      cmdList.bind_uniform_buffer(0, textRes.ubo);

      cmdList.bind_texture(1, atlas.texture());

      cmdList.draw_primitives(static_cast<int>(textRes.vertexCount), 0);
   }

   void draw_ui(graphics_api::CommandList& cmdList)
   {
      this->process_pending_updates();

      const auto resolution = this->framebuffer("ui"_name).resolution();
      m_rectangleRenderer.begin_render(cmdList);
      for (const auto& rect : m_rectangleResources | std::views::values) {
         m_rectangleRenderer.draw(cmdList, rect, resolution);
      }

      cmdList.bind_pipeline(m_textPipeline);

      for (const auto& [name, textRes] : m_textResources) {
         this->draw_text(cmdList, name, textRes);
      }
   }

 private:
   graphics_api::Device& m_device;
   resource::ResourceManager& m_resourceManager;
   ui_core::Viewport& m_viewport;
   RectangleRenderer& m_rectangleRenderer;
   graphics_api::Pipeline& m_textPipeline;
   std::map<Name, TextObject> m_textResources;
   std::vector<Name> m_textChanges{};
   std::map<Name, Rectangle> m_rectangleResources;

   ui_core::Viewport::OnAddedTextDel::Sink<UserInterfaceResources> m_onAddedTextSink;
   ui_core::Viewport::OnTextChangeContentDel::Sink<UserInterfaceResources> m_onTextChangeContentSink;
   ui_core::Viewport::OnAddedRectangleDel::Sink<UserInterfaceResources> m_onAddedRectangleSink;
};

UserInterface::UserInterface(graphics_api::Device& device, resource::ResourceManager& resourceManager, ui_core::Viewport& viewport) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_viewport(viewport),
    m_textureRenderTarget(GAPI_CHECK(
       graphics_api::RenderTargetBuilder(m_device)
          .attachment("user_interface"_name, AttachmentAttribute::Color | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage,
                      GAPI_FORMAT(RGBA, Float16), graphics_api::SampleCount::Single)
          .build())),
    m_textPipeline(
       GAPI_CHECK(graphics_api::GraphicsPipelineBuilder(device, m_textureRenderTarget)
                     .fragment_shader(resourceManager.get("text.fshader"_rc))
                     .vertex_shader(resourceManager.get("text.vshader"_rc))
                     // Vertex description
                     .begin_vertex_layout<render_core::GlyphVertex>()
                     .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(render_core::GlyphVertex, position))
                     .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(render_core::GlyphVertex, texCoord))
                     .end_vertex_layout()
                     // Descriptor layout
                     .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader)
                     .descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader)
                     .push_constant(graphics_api::PipelineStage::FragmentShader, sizeof(TextColorConstant))
                     .enable_depth_test(false)
                     .enable_blending(true)
                     .use_push_descriptors(true)
                     .vertex_topology(graphics_api::VertexTopology::TriangleList)
                     .build())),
    m_rectangleRenderer(device, m_textureRenderTarget, m_resourceManager)
{
}

graphics_api::WorkTypeFlags UserInterface::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

std::unique_ptr<render_core::NodeFrameResources> UserInterface::create_node_resources()
{
   auto result = std::make_unique<UserInterfaceResources>(m_device, m_resourceManager, m_viewport, m_rectangleRenderer, m_textPipeline);
   result->add_render_target("ui"_name, m_textureRenderTarget);
   return result;
}

void UserInterface::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                                    graphics_api::CommandList& cmdList)
{
   std::array<graphics_api::ClearValue, 1> clearValues{
      graphics_api::Color{0, 0, 0, 0},
   };

   auto& uiResources = dynamic_cast<UserInterfaceResources&>(resources);

   auto& framebuffer = resources.framebuffer("ui"_name);
   cmdList.begin_render_pass(framebuffer, clearValues);

   uiResources.draw_ui(cmdList);

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
