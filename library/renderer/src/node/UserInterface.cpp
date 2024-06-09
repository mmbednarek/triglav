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
   UserInterfaceResources(ui_core::Viewport& viewport, RectangleRenderer& rectangleRenderer, TextRenderer& textRenderer) :
       m_viewport(viewport),
       m_rectangleRenderer(rectangleRenderer),
       m_textRenderer(textRenderer),
       m_onAddedTextSink(viewport.OnAddedText.connect<&UserInterfaceResources::on_added_text>(this)),
       m_onTextChangeContentSink(viewport.OnTextChangeContent.connect<&UserInterfaceResources::on_text_content_change>(this)),
       m_onAddedRectangleSink(viewport.OnAddedRectangle.connect<&UserInterfaceResources::on_added_rectangle>(this))
   {
   }

   void on_added_text(Name id, const ui_core::Text& textObj)
   {
      m_textResources.emplace(id, m_textRenderer.create_text_object(textObj.glyphAtlas, textObj.content));
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
      auto& textRes = m_textResources.at(name);
      m_textRenderer.update_text(textRes, textObj.content);
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
      m_textRenderer.draw_text(cmdList, textRes, glm::vec2{viewportWidth, viewportHeight}, textObj.position, textObj.color);
   }

   void draw_ui(graphics_api::CommandList& cmdList)
   {
      this->process_pending_updates();

      const auto resolution = this->framebuffer("ui"_name).resolution();
      m_rectangleRenderer.begin_render(cmdList);
      for (const auto& rect : m_rectangleResources | std::views::values) {
         m_rectangleRenderer.draw(cmdList, rect, resolution);
      }

      m_textRenderer.bind_pipeline(cmdList);

      for (const auto& [name, textRes] : m_textResources) {
         this->draw_text(cmdList, name, textRes);
      }
   }

 private:
   ui_core::Viewport& m_viewport;
   RectangleRenderer& m_rectangleRenderer;
   TextRenderer& m_textRenderer;
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
    m_textRenderer(m_device, m_resourceManager, m_textureRenderTarget),
    m_rectangleRenderer(device, m_textureRenderTarget, m_resourceManager)
{
}

graphics_api::WorkTypeFlags UserInterface::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

std::unique_ptr<render_core::NodeFrameResources> UserInterface::create_node_resources()
{
   auto result = std::make_unique<UserInterfaceResources>(m_viewport, m_rectangleRenderer, m_textRenderer);
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
