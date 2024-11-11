#include "InfoDialog.hpp"

#include "triglav/Name.hpp"

#include <array>
#include <span>

namespace triglav::renderer {

using namespace name_literals;
using namespace std::string_view_literals;

constexpr auto g_leftOffset = 16.0f;
constexpr auto g_topOffset = 16.0f;

constexpr std::array g_metricsLabels{
   std::tuple{"info_dialog/metrics/fps"_name, "info_dialog/metrics/fps/value"_name, "Framerate"sv},
   std::tuple{"info_dialog/metrics/fps_min"_name, "info_dialog/metrics/fps_min/value"_name, "Framerate Min"sv},
   std::tuple{"info_dialog/metrics/fps_max"_name, "info_dialog/metrics/fps_max/value"_name, "Framerate Max"sv},
   std::tuple{"info_dialog/metrics/fps_avg"_name, "info_dialog/metrics/fps_avg/value"_name, "Framerate Avg"sv},
   std::tuple{"info_dialog/metrics/gbuffer_triangles"_name, "info_dialog/metrics/gbuffer_triangles/value"_name, "GBuffer Triangles"sv},
   std::tuple{"info_dialog/metrics/gbuffer_gpu_time"_name, "info_dialog/metrics/gbuffer_gpu_time/value"_name, "GBuffer Render Time"sv},
   std::tuple{"info_dialog/metrics/shading_triangles"_name, "info_dialog/metrics/shading_triangles/value"_name, "Shading Triangles"sv},
   std::tuple{"info_dialog/metrics/shading_gpu_time"_name, "info_dialog/metrics/shading_gpu_time/value"_name, "Shading Render Time"sv},
   std::tuple{"info_dialog/metrics/ray_tracing_gpu_time"_name, "info_dialog/metrics/ray_tracing_gpu_time/value"_name, "Ray Tracing Time"sv},
};

constexpr std::array g_locationLabels{
   std::tuple{"info_dialog/location/position"_name, "info_dialog/location/position/value"_name, "Position"sv},
   std::tuple{"info_dialog/location/orientation"_name, "info_dialog/location/orientation/value"_name, "Orientation"sv},
};

constexpr std::array g_featureLabels{
   std::tuple{"info_dialog/features/ao"_name, "info_dialog/features/ao/value"_name, "Ambient Occlusion"sv},
   std::tuple{"info_dialog/features/aa"_name, "info_dialog/features/aa/value"_name, "Anti-Aliasing"sv},
   std::tuple{"info_dialog/features/bloom"_name, "info_dialog/features/bloom/value"_name, "Bloom"sv},
   std::tuple{"info_dialog/features/debug_lines"_name, "info_dialog/features/debug_lines/value"_name, "Debug Lines"sv},
   std::tuple{"info_dialog/features/smooth_camera"_name, "info_dialog/features/smooth_camera/value"_name, "Smooth Camera"sv},
};

constexpr std::array g_labelGroups{
   std::tuple{"info_dialog/metrics"_name, "Metrics", std::span{g_metricsLabels.data(), g_metricsLabels.size()}},
   std::tuple{"info_dialog/location"_name, "Location", std::span{g_locationLabels.data(), g_locationLabels.size()}},
   std::tuple{"info_dialog/features"_name, "Features", std::span{g_featureLabels.data(), g_featureLabels.size()}},
};

InfoDialog::InfoDialog(ui_core::Viewport& viewport, resource::ResourceManager& resourceManager, GlyphCache& glyphCache) :
    m_viewport(viewport),
    m_resourceManager(resourceManager),
    m_glyphCache(glyphCache)
{
}

void InfoDialog::initialize()
{
   m_viewport.add_rectangle("info_dialog/bg"_name, ui_core::Rectangle{.rect{5.0f, 5.0f, 380.0f, 640.0f}});

   m_position = {g_leftOffset, g_topOffset};

   this->add_text("info_dialog/title"_name, "Triglav Render Demo", "cantarell/bold.typeface"_rc, 24, {1.0f, 1.0f, 1.0f, 1.0f},
                  Alignment::Top, 0.0f);

   m_position.y += 10.0f;

   for (const auto& labelGroups : g_labelGroups) {
      this->add_text(std::get<0>(labelGroups), std::get<1>(labelGroups), "segoeui/bold.typeface"_rc, 20, {1.0f, 1.0f, 0.4f, 1.0f},
                     Alignment::Top, 16.0f);

      for (const auto& label : std::get<2>(labelGroups)) {
         this->add_text(std::get<0>(label), std::get<2>(label), "segoeui.typeface"_rc, 18, {1.0f, 1.0f, 1.0f, 1.0f}, Alignment::Top, 16.0f);
         this->add_text(std::get<1>(label), "none", "segoeui.typeface"_rc, 18, {1.0f, 1.0f, 0.4f, 1.0f}, Alignment::Left, 16.0f);
      }
   }
}

render_core::TextMetric InfoDialog::measure_text(const ui_core::Text& text)
{
   auto& atlas = m_glyphCache.find_glyph_atlas(GlyphProperties{text.typefaceName, text.fontSize});
   return atlas.measure_text(text.content);
}

void InfoDialog::add_text(const Name id, const std::string_view content, TypefaceName typefaceName, int fontSize, glm::vec4 color,
                          Alignment align, float offset)
{
   ui_core::Text textObj{
      .content = std::string{content},
      .typefaceName = typefaceName,
      .fontSize = fontSize,
      .color = color,
   };

   auto measure = this->measure_text(textObj);

   switch (align) {
   case Alignment::Top:
      m_position.x = g_leftOffset;
      m_position.y += measure.height + offset;
      break;
   case Alignment::Left:
      m_position.x += offset;
      break;
   }
   textObj.position = m_position;
   m_position.x += measure.width;

   m_viewport.add_text(id, std::move(textObj));
}

}// namespace triglav::renderer
