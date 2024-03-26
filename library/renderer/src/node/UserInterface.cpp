#include "UserInterface.h"

#include <ranges>

using triglav::graphics_api::AttachmentAttribute;

namespace triglav::renderer::node {

using namespace name_literals;

UserInterface::UserInterface(graphics_api::Device &device, const graphics_api::Resolution resolution,
                             resource::ResourceManager &resourceManager) :
    m_resourceManager(resourceManager),
    m_textureRenderTarget(GAPI_CHECK(graphics_api::RenderTargetBuilder(device).attachment(
            AttachmentAttribute::Color | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage,
            GAPI_FORMAT(RGBA, Float16), graphics_api::SampleCount::Single).build())),
    m_framebuffer(GAPI_CHECK(m_textureRenderTarget.create_framebuffer(resolution))),
    m_rectangleRenderer(device, m_textureRenderTarget, m_resourceManager),
    m_textRenderer(device, m_textureRenderTarget, m_resourceManager),
    m_rectangle(m_rectangleRenderer.create_rectangle(glm::vec4{5.0f, 5.0f, 480.0f, 250.0f})),
    m_titleLabel(m_textRenderer.create_text_object(
            m_resourceManager.get<ResourceType::GlyphAtlas>("cantarell/bold.glyphs"_name),
            "Triglav Render Demo"))
{
   m_textRenderer.update_resolution(m_framebuffer.resolution());
}

graphics_api::WorkTypeFlags UserInterface::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void UserInterface::record_commands(graphics_api::CommandList &cmdList)
{
   std::array<graphics_api::ClearValue, 1> clearValues{
           graphics_api::Color{0, 0, 0, 0},
   };
   cmdList.begin_render_pass(m_framebuffer, clearValues);

   m_rectangleRenderer.begin_render(cmdList);
   m_rectangleRenderer.draw(cmdList, m_rectangle, m_framebuffer.resolution());

   m_textRenderer.begin_render(cmdList);

   auto textY = 16.0f + m_titleLabel.metric.height;
   m_textRenderer.draw_text_object(cmdList, m_titleLabel, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
   textY += 12.0f;

   for (const auto &property : std::views::values(m_properties)) {
      textY += 12.0f + property.label.metric.height;
      m_textRenderer.draw_text_object(cmdList, property.label, {16.0f, textY}, {1.0f, 1.0f, 1.0f});
      m_textRenderer.draw_text_object(cmdList, property.value,
                                      {16.0f + property.label.metric.width + 8.0f, textY},
                                      {1.0f, 1.0f, 0.4f});
   }

   cmdList.end_render_pass();
}

void UserInterface::add_label(const NameID id, const std::string_view name)
{
   m_resourceManager.get<ResourceType::GlyphAtlas>("cantarell.glyphs"_name);
   m_properties.emplace(
           id, TextProperty{m_textRenderer.create_text_object(
                                    m_resourceManager.get<ResourceType::GlyphAtlas>("cantarell.glyphs"_name),
                                    name),
                            m_textRenderer.create_text_object(
                                    m_resourceManager.get<ResourceType::GlyphAtlas>("cantarell.glyphs"_name),
                                    "none")});
}

void UserInterface::set_value(const NameID id, const std::string_view value)
{
   const auto &atlas = m_resourceManager.get<ResourceType::GlyphAtlas>("cantarell.glyphs"_name);
   m_textRenderer.update_text_object(atlas, m_properties.at(id).value, value);
}

graphics_api::Texture &UserInterface::texture()
{
   return m_framebuffer.texture(0);
}

}// namespace triglav::renderer::node
