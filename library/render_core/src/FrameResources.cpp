#include "FrameResources.h"

#include <ranges>

namespace triglav::render_core {

void FrameResources::add_render_target(NameID idenfitier, graphics_api::RenderTarget& renderTarget)
{
   m_renderTargets.emplace(idenfitier, &renderTarget);
}

void FrameResources::update_resolution(const graphics_api::Resolution &resolution)
{
   m_framebuffers.clear();

   for (const auto &[name, target] : m_renderTargets) {
      m_framebuffers.emplace(name, GAPI_CHECK(target->create_framebuffer(resolution)));

      u32 index{};
      for (const auto &attachment : target->attachments()) {
         m_textures.emplace(attachment.identifier, &m_framebuffers.at(name).texture(index));
         ++index;
      }
   }
}

graphics_api::Framebuffer &FrameResources::framebuffer(const NameID identifier)
{
   return m_framebuffers.at(identifier);
}

graphics_api::Texture &FrameResources::texture(const NameID identifier) const
{
   return *m_textures.at(identifier);
}

}// namespace triglav::render_core
