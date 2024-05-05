#include "InfoDialog.h"

#include "triglav/Name.hpp"

#include <array>
#include <span>

namespace triglav::renderer {

using namespace name_literals;
using namespace std::string_view_literals;

constexpr auto g_leftOffset = 16.0f;
constexpr auto g_topOffset  = 16.0f;

constexpr std::array g_metricsLabels{
        std::tuple{      "info_dialog/metrics/fps"_name_id,       "info_dialog/metrics/fps/value"_name_id,"Framerate"sv                            },
        std::tuple{"info_dialog/metrics/triangles"_name_id, "info_dialog/metrics/triangles/value"_name_id,
                   "GBuffer Triangles"sv  },
        std::tuple{ "info_dialog/metrics/gpu_time"_name_id,  "info_dialog/metrics/gpu_time/value"_name_id,
                   "GBuffer Render Time"sv},
};

constexpr std::array g_locationLabels{
        std::tuple{   "info_dialog/location/position"_name_id,"info_dialog/location/position/value"_name_id,
                   "Position"sv                                                     },
        std::tuple{"info_dialog/location/orientation"_name_id,
                   "info_dialog/location/orientation/value"_name_id, "Orientation"sv},
};

constexpr std::array g_featureLabels{
        std::tuple{         "info_dialog/features/ao"_name_id,"info_dialog/features/ao/value"_name_id,
                   "Ambient Occlusion"sv                                            },
        std::tuple{         "info_dialog/features/aa"_name_id, "info_dialog/features/aa/value"_name_id,
                   "Anti-Aliasing"sv                                                },
        std::tuple{"info_dialog/features/debug_lines"_name_id,
                   "info_dialog/features/debug_lines/value"_name_id, "Debug Lines"sv},
};

constexpr std::array g_labelGroups{
        std::tuple{ "info_dialog/metrics"_name_id,  "Metrics",
                   std::span{g_metricsLabels.data(), g_metricsLabels.size()}  },
        std::tuple{"info_dialog/location"_name_id, "Location",
                   std::span{g_locationLabels.data(), g_locationLabels.size()}},
        std::tuple{"info_dialog/features"_name_id, "Features",
                   std::span{g_featureLabels.data(), g_featureLabels.size()}  },
};

InfoDialog::InfoDialog(ui_core::Viewport &viewport, resource::ResourceManager &resourceManager) :
    m_viewport(viewport),
    m_resourceManager(resourceManager)
{
}

void InfoDialog::initialize()
{
   m_viewport.add_rectangle("info_dialog/bg"_name_id, ui_core::Rectangle{
                                                              .rect{5.0f, 5.0f, 380.0f, 380.0f}
   });

   m_position = {g_leftOffset, g_topOffset};

   this->add_text("info_dialog/title"_name_id, "Triglav Render Demo", "cantarell/bold.glyphs"_name,
                  {1.0f, 1.0f, 1.0f, 1.0f}, Alignment::Top, 0.0f);

   for (const auto &labelGroups : g_labelGroups) {
      this->add_text(std::get<0>(labelGroups), std::get<1>(labelGroups), "segoeui/bold/20.glyphs"_name,
                     {1.0f, 1.0f, 0.4f, 1.0f}, Alignment::Top, 16.0f);

      for (const auto &label : std::get<2>(labelGroups)) {
         this->add_text(std::get<0>(label), std::get<2>(label), "segoeui/regular/18.glyphs"_name,
                        {1.0f, 1.0f, 1.0f, 1.0f}, Alignment::Top, 16.0f);
         this->add_text(std::get<1>(label), "none", "segoeui/regular/18.glyphs"_name,
                        {1.0f, 1.0f, 0.4f, 1.0f}, Alignment::Left, 16.0f);
      }
   }
}

render_core::TextMetric InfoDialog::measure_text(const ui_core::Text &text)
{
   auto &atlas = m_resourceManager.get<ResourceType::GlyphAtlas>(text.glyphAtlas);
   return atlas.measure_text(text.content);
}

void InfoDialog::add_text(const NameID id, const std::string_view content,
                          const GlyphAtlasName glyphAtlasName, glm::vec4 color, Alignment align, float offset)
{
   ui_core::Text textObj{
           .content    = std::string{content},
           .glyphAtlas = glyphAtlasName,
           .color      = color,
   };

   auto measure = this->measure_text(textObj);

   switch (align) {
   case Alignment::Top:
      m_position.x = g_leftOffset;
      m_position.y += measure.height + offset;
      break;
   case Alignment::Left: m_position.x += offset; break;
   }
   textObj.position = m_position;
   m_position.x += measure.width;

   m_viewport.add_text(id, std::move(textObj));
}

}// namespace triglav::renderer