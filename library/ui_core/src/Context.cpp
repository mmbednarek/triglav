#include "Context.hpp"

namespace triglav::ui_core {

Context::Context(Viewport& m_viewport, render_core::GlyphCache& m_glyph_cache) :
    m_viewport(m_viewport),
    m_glyphCache(m_glyph_cache)
{
}

Viewport& Context::viewport() const
{
   return m_viewport;
}

render_core::GlyphCache& Context::glyph_cache() const
{
   return m_glyphCache;
}

}// namespace triglav::ui_core
