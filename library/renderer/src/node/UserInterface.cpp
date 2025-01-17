#include "UserInterface.hpp"

#include "ProcessGlyphs.hpp"

#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/render_core/GlyphAtlas.hpp"
#include "triglav/ui_core/Viewport.hpp"

#include <ranges>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>

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
   using Self = UserInterfaceResources;

   UserInterfaceResources(ui_core::Viewport& viewport, RectangleRenderer& rectangleRenderer) :
       m_rectangleRenderer(rectangleRenderer),
       TG_CONNECT(viewport, OnAddedRectangle, on_added_rectangle)
   {
   }

   void on_added_rectangle(Name id, const ui_core::Rectangle& rectObj)
   {
      m_rectangleResources.emplace(id, m_rectangleRenderer.create_rectangle(rectObj.rect, glm::vec4{0.0f, 0.0f, 0.0f, 0.8f}));
   }

   void draw_ui(graphics_api::CommandList& cmdList)
   {
      const auto resolution = this->framebuffer("ui"_name).resolution();
      m_rectangleRenderer.begin_render(cmdList);
      for (const auto& rect : m_rectangleResources | std::views::values) {
         m_rectangleRenderer.draw(cmdList, rect, resolution);
      }
   }

 private:
   RectangleRenderer& m_rectangleRenderer;
   std::map<Name, TextObject> m_textResources;
   std::vector<Name> m_textChanges{};
   std::map<Name, Rectangle> m_rectangleResources;

   TG_SINK(ui_core::Viewport, OnAddedRectangle);
};


UserInterface::UserInterface(graphics_api::Device& device, resource::ResourceManager& resourceManager, ui_core::Viewport& viewport,
                             GlyphCache& glyphCache) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_viewport(viewport),
    m_textureRenderTarget(GAPI_CHECK(
       graphics_api::RenderTargetBuilder(m_device)
          .attachment("user_interface"_name, AttachmentAttribute::Color | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage,
                      GAPI_FORMAT(RGBA, Float16), graphics_api::SampleCount::Single)
          .build())),
    m_textRenderer(m_device, m_resourceManager, m_textureRenderTarget, glyphCache),
    m_rectangleRenderer(device, m_textureRenderTarget, m_resourceManager),
    m_ubo(m_device)
{
}

graphics_api::WorkTypeFlags UserInterface::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

std::unique_ptr<render_core::NodeFrameResources> UserInterface::create_node_resources()
{
   auto result = std::make_unique<UserInterfaceResources>(m_viewport, m_rectangleRenderer);
   result->add_render_target("ui"_name, m_textureRenderTarget);
   return result;
}

void UserInterface::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& resources,
                                    graphics_api::CommandList& cmdList)
{
   std::array clearValues{
      graphics_api::ClearValue{graphics_api::Color{0, 0, 0, 0}},
   };

   auto& uiResources = dynamic_cast<UserInterfaceResources&>(resources);

   auto& framebuffer = resources.framebuffer("ui"_name);
   cmdList.begin_render_pass(framebuffer, clearValues);

   uiResources.draw_ui(cmdList);

   auto& processGlyphsRes = dynamic_cast<ProcessGlyphsResources&>(frameResources.node("process_glyphs"_name));
   processGlyphsRes.draw(cmdList, m_textRenderer.pipeline(), {framebuffer.resolution().width, framebuffer.resolution().height});

   cmdList.end_render_pass();
}

}// namespace triglav::renderer::node
