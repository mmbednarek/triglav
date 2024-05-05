#pragma once

#include "triglav/render_core/GlyphAtlas.h"
#include "triglav/resource/ResourceManager.h"
#include "triglav/ui_core/Viewport.h"

namespace triglav::renderer {

class InfoDialog {
 public:
   InfoDialog(ui_core::Viewport &viewport, resource::ResourceManager &resourceManager);

   void initialize();

 private:
   enum class Alignment {
      Top,
      Left
   };

   [[nodiscard]] render_core::TextMetric measure_text(const ui_core::Text& text);
   void add_text(Name id, std::string_view content, GlyphAtlasName glyphAtlasName, glm::vec4 color, Alignment align, float offset);

   ui_core::Viewport &m_viewport;
   resource::ResourceManager &m_resourceManager;

   glm::vec2 m_position;
};

}