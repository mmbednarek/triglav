#include "Context.hpp"

namespace triglav::ui_core {

Context::Context(Viewport& viewport, render_core::GlyphCache& glyphCache, resource::ResourceManager& resourceManager) :
    m_viewport(viewport),
    m_glyphCache(glyphCache),
    m_resourceManager(resourceManager)
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

resource::ResourceManager& Context::resource_manager() const
{
   return m_resourceManager;
}

}// namespace triglav::ui_core
