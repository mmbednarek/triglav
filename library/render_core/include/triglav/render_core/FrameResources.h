#pragma once

#include "triglav/graphics_api/RenderTarget.h"

#include <map>

namespace triglav::render_core {

class FrameResources {
public:
  void add_render_target(NameID idenfitier, graphics_api::RenderTarget& renderTarget);
  void update_resolution(const graphics_api::Resolution& resolution);

  [[nodiscard]] graphics_api::Framebuffer& framebuffer(NameID identifier);
  [[nodiscard]] graphics_api::Texture& texture(NameID identifier) const;

private:
  std::map<NameID, graphics_api::RenderTarget*> m_renderTargets;
  std::map<NameID, graphics_api::Framebuffer> m_framebuffers;
  std::map<NameID, graphics_api::Texture*> m_textures;

};

}