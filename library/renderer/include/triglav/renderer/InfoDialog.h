#pragma once

#include "GlyphCache.h"

#include "triglav/render_core/GlyphAtlas.h"
#include "triglav/resource/ResourceManager.h"
#include "triglav/ui_core/Viewport.h"

namespace triglav::renderer {

class InfoDialog
{
 public:
   InfoDialog(ui_core::Viewport& viewport, resource::ResourceManager& resourceManager, GlyphCache& glyphCache);

   void initialize();

 private:
   enum class Alignment
   {
      Top,
      Left
   };

   [[nodiscard]] render_core::TextMetric measure_text(const ui_core::Text& text);
   void add_text(Name id, std::string_view content, TypefaceName typefaceName, int fontSize, glm::vec4 color, Alignment align, float offset);

   ui_core::Viewport& m_viewport;
   resource::ResourceManager& m_resourceManager;
   GlyphCache& m_glyphCache;

   glm::vec2 m_position;
};

}// namespace triglav::renderer