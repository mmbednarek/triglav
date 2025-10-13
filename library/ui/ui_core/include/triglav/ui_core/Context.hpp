#pragma once

namespace triglav::render_core {
class GlyphCache;
}

namespace triglav::resource {
class ResourceManager;
}

namespace triglav::ui_core {

class Viewport;

class Context
{
 public:
   Context(Viewport& viewport, render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager);

   [[nodiscard]] Viewport& viewport() const;
   [[nodiscard]] render_core::GlyphCache& glyph_cache() const;
   [[nodiscard]] resource::ResourceManager& resource_manager() const;

 private:
   Viewport& m_viewport;
   render_core::GlyphCache& m_glyphCache;
   resource::ResourceManager& m_resourceManager;
};

}// namespace triglav::ui_core
