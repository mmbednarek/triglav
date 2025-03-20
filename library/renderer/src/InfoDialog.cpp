#include "InfoDialog.hpp"

#include "triglav/Name.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <array>
#include <span>

namespace triglav::renderer {

using namespace name_literals;
using namespace std::string_view_literals;

constexpr auto g_leftOffset = 16.0f;
constexpr auto g_topOffset = 16.0f;

constexpr std::array g_metricsLabels{
   std::tuple{"metrics.fps"_name, "Framerate"sv},
   std::tuple{"metrics.fps_min"_name, "Framerate Min"sv},
   std::tuple{"metrics.fps_max"_name, "Framerate Max"sv},
   std::tuple{"metrics.fps_avg"_name, "Framerate Avg"sv},
   std::tuple{"metrics.triangles"_name, "Triangle Count"sv},
   std::tuple{"metrics.gpu_time"_name, "GPU Render Time"sv},
};

constexpr std::array g_locationLabels{
   std::tuple{"location.position"_name, "Position"sv},
   std::tuple{"location.orientation"_name, "Orientation"sv},
};

constexpr std::array g_featureLabels{
   std::tuple{"features.ao"_name, "Ambient Occlusion"sv},    std::tuple{"features.aa"_name, "Anti-Aliasing"sv},
   std::tuple{"features.shadows"_name, "Shadows"sv},         std::tuple{"features.bloom"_name, "Bloom"sv},
   std::tuple{"features.debug_lines"_name, "Debug Lines"sv}, std::tuple{"features.smooth_camera"_name, "Smooth Camera"sv},
};

constexpr std::array g_labelGroups{
   std::tuple{"Metrics", std::span{g_metricsLabels.data(), g_metricsLabels.size()}},
   std::tuple{"Location", std::span{g_locationLabels.data(), g_locationLabels.size()}},
   std::tuple{"Features", std::span{g_featureLabels.data(), g_featureLabels.size()}},
};

InfoDialog::InfoDialog(ui_core::Context& context, ConfigManager& configManager) :
    m_context(context),
    m_configManager(configManager),
    m_rootBox(context,
              ui_core::RectBox::State{
                 .color = {0.0, 0.0f, 0.0f, 0.75f},
              }),
    TG_CONNECT(m_configManager, OnPropertyChanged, on_config_property_changed)
{
   auto& viewport = m_rootBox.create_content<ui_core::VerticalLayout>({
      .padding = {20.0f, 20.0f, 80.0f, 20.0f},
      .separation = {10.0f},
   });

   viewport.create_child<ui_core::TextBox>({
      .fontSize = 24,
      .typeface = "cantarell/bold.typeface"_rc,
      .content = "Triglav Render Demo",
      .color = {1.0f, 1.0f, 1.0f, 1.0f},
   });

   viewport.create_child<ui_core::EmptySpace>({
      .size = {1.0f, 10.0f},
   });

   for (const auto& labelGroups : g_labelGroups) {
      viewport.create_child<ui_core::TextBox>({
         .fontSize = 20,
         .typeface = "segoeui/bold.typeface"_rc,
         .content = std::get<0>(labelGroups),
         .color = {1.0f, 1.0f, 0.4f, 1.0f},
      });

      auto& labelBox = viewport.create_child<ui_core::HorizontalLayout>({
         .padding = {0.0f, 10.0f, 0.0f, 10.0f},
         .separation = {10.0f},
      });

      auto& leftPanel = labelBox.create_child<ui_core::VerticalLayout>({
         .padding = {0, 0.0f, 5.0f, 0.0f},
         .separation = {14.0f},
      });

      auto& rightPanel = labelBox.create_child<ui_core::VerticalLayout>({
         .padding = {5.0f, 0.0f, 0, 0.0f},
         .separation = {14.0f},
      });

      for (const auto& label : std::get<1>(labelGroups)) {
         leftPanel.create_child<ui_core::TextBox>({
            .fontSize = 18,
            .typeface = "segoeui.typeface"_rc,
            .content = std::string{std::get<1>(label)},
            .color = {1.0f, 1.0f, 1.0f, 1.0f},
         });

         m_values[std::get<0>(label)] = &rightPanel.create_child<ui_core::TextBox>({
            .fontSize = 18,
            .typeface = "segoeui.typeface"_rc,
            .content = "none",
            .color = {1.0f, 1.0f, 0.4f, 1.0f},
         });
      }
   }
}

void InfoDialog::initialize()
{
   const auto size = m_rootBox.desired_size({600, 1200});
   m_rootBox.add_to_viewport({20, 20, size.x, size.y});
}

void InfoDialog::set_fps(const float value) const
{
   const auto framerateStr = std::format("{:.1f}", value);
   m_values.at("metrics.fps"_name)->set_content(framerateStr);
}

void InfoDialog::set_min_fps(const float value) const
{
   const auto framerateStr = std::format("{:.1f}", value);
   m_values.at("metrics.fps_min"_name)->set_content(framerateStr);
}

void InfoDialog::set_max_fps(const float value) const
{
   const auto framerateMaxStr = std::format("{:.1f}", value);
   m_values.at("metrics.fps_max"_name)->set_content(framerateMaxStr);
}

void InfoDialog::set_avg_fps(const float value) const
{
   const auto framerateAvgStr = std::format("{:.1f}", value);
   m_values.at("metrics.fps_avg"_name)->set_content(framerateAvgStr);
}

void InfoDialog::set_gpu_time(const float value) const
{
   const auto gBufferGpuTimeStr = std::format("{:.2f}ms", value);
   m_values.at("metrics.gpu_time"_name)->set_content(gBufferGpuTimeStr);
}

void InfoDialog::set_triangle_count(const u32 value) const
{
   const auto primitiveCountStr = std::format("{}", value);
   m_values.at("metrics.triangles"_name)->set_content(primitiveCountStr);
}

void InfoDialog::set_camera_pos(const Vector3 value) const
{
   const auto positionStr = std::format("{:.2f}, {:.2f}, {:.2f}", value.x, value.y, value.z);
   m_values.at("location.position"_name)->set_content(positionStr);
}

void InfoDialog::set_orientation(const Vector2 value) const
{
   const auto orientationStr = std::format("{:.2f}, {:.2f}", value.x, value.y);
   m_values.at("location.orientation"_name)->set_content(orientationStr);
}

void InfoDialog::on_config_property_changed(const ConfigProperty property, const Config& config)
{
   switch (property) {
   case ConfigProperty::AmbientOcclusion:
      m_values.at("features.ao"_name)->set_content(ambient_occlusion_method_to_string(config.ambientOcclusion));
      break;
   case ConfigProperty::Antialiasing:
      m_values.at("features.aa"_name)->set_content(antialiasing_method_to_string(config.antialiasing));
      break;
   case ConfigProperty::ShadowCasting:
      m_values.at("features.shadows"_name)->set_content(shadow_casting_method_to_string(config.shadowCasting));
      break;
   case ConfigProperty::IsBloomEnabled:
      m_values.at("features.bloom"_name)->set_content(config.isBloomEnabled ? "On" : "Off");
      break;
   case ConfigProperty::IsSmoothCameraEnabled:
      m_values.at("features.smooth_camera"_name)->set_content(config.isSmoothCameraEnabled ? "On" : "Off");
      break;
   default:
      break;
   }
}

void InfoDialog::init_config_labels() const
{
   const auto& config = m_configManager.config();
   m_values.at("features.ao"_name)->set_content(ambient_occlusion_method_to_string(config.ambientOcclusion));
   m_values.at("features.aa"_name)->set_content(antialiasing_method_to_string(config.antialiasing));
   m_values.at("features.shadows"_name)->set_content(shadow_casting_method_to_string(config.shadowCasting));
   m_values.at("features.bloom"_name)->set_content(config.isBloomEnabled ? "On" : "Off");
   m_values.at("features.smooth_camera"_name)->set_content(config.isSmoothCameraEnabled ? "On" : "Off");
}

}// namespace triglav::renderer
