#pragma once

namespace triglav::render_core {
class GlyphCache;
}

namespace triglav::ui_core {

class Viewport;

class Context
{
 public:
   Context(Viewport& m_viewport, render_core::GlyphCache& m_glyph_cache);

   [[nodiscard]] Viewport& viewport() const;
   [[nodiscard]] render_core::GlyphCache& glyph_cache() const;

 private:
   Viewport& m_viewport;
   render_core::GlyphCache& m_glyphCache;
};

}// namespace triglav::ui_core
