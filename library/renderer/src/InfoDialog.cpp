#include "InfoDialog.hpp"

#include "triglav/Format.hpp"
#include "triglav/Name.hpp"
#include "triglav/desktop/ISurface.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/Button.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/HideableWidget.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <array>
#include <span>
#include <spdlog/spdlog.h>

namespace triglav::renderer {

using namespace name_literals;
using namespace string_literals;

constexpr std::array g_metricsLabels{
   std::tuple{"metrics.fps"_name, "Framerate"_strv},
   std::tuple{"metrics.fps_min"_name, "Framerate Min"_strv},
   std::tuple{"metrics.fps_max"_name, "Framerate Max"_strv},
   std::tuple{"metrics.fps_avg"_name, "Framerate Avg"_strv},
   std::tuple{"metrics.triangles"_name, "Triangle Count"_strv},
   std::tuple{"metrics.gpu_time"_name, "GPU Render Time"_strv},
};

constexpr std::array g_locationLabels{
   std::tuple{"location.position"_name, "Position"_strv},
   std::tuple{"location.orientation"_name, "Orientation"_strv},
};

constexpr std::array g_featureLabels{
   std::tuple{"features.ao"_name, "Ambient Occlusion"_strv},    std::tuple{"features.aa"_name, "Anti-Aliasing"_strv},
   std::tuple{"features.shadows"_name, "Shadows"_strv},         std::tuple{"features.bloom"_name, "Bloom"_strv},
   std::tuple{"features.debug_lines"_name, "Debug Lines"_strv}, std::tuple{"features.smooth_camera"_name, "Smooth Camera"_strv},
};

constexpr std::array g_labelGroups{
   std::tuple{"Metrics"_strv, std::span{g_metricsLabels.data(), g_metricsLabels.size()}},
   std::tuple{"Location"_strv, std::span{g_locationLabels.data(), g_locationLabels.size()}},
   std::tuple{"Features"_strv, std::span{g_featureLabels.data(), g_featureLabels.size()}},
};

class HideablePanel final : public ui_core::IWidget
{
 public:
   using Self = HideablePanel;

   struct State
   {
      Vector4 padding;
      float separation;
      String label;
      desktop::ISurface* surface;
   };

   HideablePanel(ui_core::Context& ctx, const State& state, ui_core::IWidget* parent) :
       m_state(state),
       m_parent(parent),
       m_verticalLayout(ctx,
                        {
                           .padding = state.padding,
                           .separation = state.separation,
                        },
                        this),
       m_labelBox(m_verticalLayout.create_child<ui_core::Button>({})),
       m_labelLayout(m_labelBox.create_content<ui_core::HorizontalLayout>({
          .separation = 10.0f,
          .gravity = ui_core::HorizontalAlignment::Center,
       })),
       m_image(m_labelLayout.create_child<ui_core::Image>({
          .texture = "texture/ui_images.tex"_rc,
          .maxSize = Vector2{20, 20},
          .region = Vector4{400, 0, 200, 200},
       })),
       m_labelText(m_labelLayout.create_child<ui_core::TextBox>({
          .fontSize = 20,
          .typeface = "segoeui/bold.typeface"_rc,
          .content = state.label,
          .color = {1.0f, 1.0f, 0.4f, 1.0f},
          .verticalAlignment = ui_core::VerticalAlignment::Center,
       })),
       m_content(m_verticalLayout.create_child<ui_core::HideableWidget>({
          .isHidden = false,
       })),
       TG_CONNECT(m_labelBox, OnClick, on_click),
       TG_CONNECT(m_labelBox, OnEnter, on_enter),
       TG_CONNECT(m_labelBox, OnLeave, on_leave)
   {
   }

   [[nodiscard]] Vector2 desired_size(const Vector2 parentSize) const override
   {
      return m_verticalLayout.desired_size(parentSize);
   }

   void add_to_viewport(const Vector4 dimensions) override
   {
      m_verticalLayout.add_to_viewport(dimensions);
   }

   void remove_from_viewport() override
   {
      m_verticalLayout.remove_from_viewport();
   }

   template<ui_core::ConstructableWidget TChild>
   TChild& create_child(typename TChild::State&& state)
   {
      return m_content.create_content<TChild>(std::move(state));
   }

   void on_child_state_changed(IWidget& widget) override
   {
      if (m_parent != nullptr) {
         m_parent->on_child_state_changed(widget);
      }
   }

   void on_event(const ui_core::Event& event) override
   {
      return m_verticalLayout.on_event(event);
   }

   void on_click(const desktop::MouseButton /*mouseButton*/) const
   {
      if (m_content.state().isHidden) {
         m_image.set_region({400, 0, 200, 200});
         m_content.set_is_hidden(false);
      } else {
         m_image.set_region({200, 0, 200, 200});
         m_content.set_is_hidden(true);
      }
   }

   void on_enter() const
   {
      m_labelText.set_color({1.0f, 1.0f, 0.8f, 1.0f});
      if (m_state.surface != nullptr) {
         m_state.surface->set_cursor_icon(desktop::CursorIcon::Hand);
      }
   }

   void on_leave() const
   {
      m_labelText.set_color({1.0f, 1.0f, 0.4f, 1.0f});
      if (m_state.surface != nullptr) {
         m_state.surface->set_cursor_icon(desktop::CursorIcon::Arrow);
      }
   }

 private:
   State m_state;
   ui_core::IWidget* m_parent;
   ui_core::VerticalLayout m_verticalLayout;
   ui_core::Button& m_labelBox;
   ui_core::HorizontalLayout& m_labelLayout;
   ui_core::Image& m_image;
   ui_core::TextBox& m_labelText;
   ui_core::HideableWidget& m_content;

   TG_SINK(ui_core::Button, OnClick);
   TG_SINK(ui_core::Button, OnEnter);
   TG_SINK(ui_core::Button, OnLeave);
};

InfoDialog::InfoDialog(ui_core::Context& context, ConfigManager& configManager, desktop::ISurface& surface) :
    m_context(context),
    m_configManager(configManager),
    m_surface(surface),
    m_rootBox(context,
              ui_core::RectBox::State{
                 .color = {0.0, 0.0f, 0.0f, 0.75f},
              },
              this),
    TG_CONNECT(m_configManager, OnPropertyChanged, on_config_property_changed)
{
   auto& viewport = m_rootBox.create_content<ui_core::VerticalLayout>({
      .padding = {25.0f, 25.0f, 25.0f, 25.0f},
      .separation = 10.0f,
   });

   auto& titleButton = viewport.create_child<ui_core::Button>({});

   auto& titleLayout = titleButton.create_content<ui_core::HorizontalLayout>({
      .separation = 5.0f,
      .gravity = ui_core::HorizontalAlignment::Center,
   });

   titleLayout.create_child<ui_core::Image>({
      .texture = "texture/ui_images.tex"_rc,
      .maxSize = Vector2{32, 32},
      .region = Vector4{0, 0, 200, 200},
   });

   m_title = &titleLayout.create_child<ui_core::TextBox>({
      .fontSize = 24,
      .typeface = "cantarell/bold.typeface"_rc,
      .content = "Triglav Render Demo",
      .color = {1.0f, 1.0f, 1.0f, 1.0f},
      .horizontalAlignment = ui_core::HorizontalAlignment::Center,
      .verticalAlignment = ui_core::VerticalAlignment::Center,
   });

   TG_CONNECT_OPT(titleButton, OnClick, on_title_clicked);
   TG_CONNECT_OPT(titleButton, OnEnter, on_title_enter);
   TG_CONNECT_OPT(titleButton, OnLeave, on_title_leave);

   viewport.create_child<ui_core::EmptySpace>({
      .size = {1.0f, 4.0f},
   });

   for (const auto& [groupLabel, group] : g_labelGroups) {
      auto& panel = viewport.create_child<HideablePanel>({
         .separation = 8.0f,
         .label = groupLabel,
         .surface = &m_surface,
      });

      auto& labelBox = panel.create_child<ui_core::HorizontalLayout>({
         .padding = {0.0f, 15.0f, 0.0f, 10.0f},
         .separation = 10.0f,
         .gravity = ui_core::HorizontalAlignment::Center,
      });

      auto& leftPanel = labelBox.create_child<ui_core::VerticalLayout>({
         .padding = {0, 0.0f, 5.0f, 0.0f},
         .separation = 10.0f,
      });

      auto& rightPanel = labelBox.create_child<ui_core::VerticalLayout>({
         .padding = {5.0f, 0.0f, 0, 0.0f},
         .separation = 10.0f,
      });

      for (const auto& [name, content] : group) {
         leftPanel.create_child<ui_core::TextBox>({
            .fontSize = 18,
            .typeface = "segoeui.typeface"_rc,
            .content = content,
            .color = {1.0f, 1.0f, 1.0f, 1.0f},
            .horizontalAlignment = ui_core::HorizontalAlignment::Right,
         });

         m_values[name] = &rightPanel.create_child<ui_core::TextBox>({
            .fontSize = 18,
            .typeface = "segoeui.typeface"_rc,
            .content = "none",
            .color = {1.0f, 1.0f, 0.4f, 1.0f},
            .horizontalAlignment = ui_core::HorizontalAlignment::Left,
         });
      }
   }
}

Vector2 InfoDialog::desired_size(const Vector2 parentSize) const
{
   return m_rootBox.desired_size(parentSize);
}

void InfoDialog::add_to_viewport(Vector4 /*dimensions*/)
{
   const auto size = this->desired_size({600, 1200});
   m_rootBox.add_to_viewport({m_dialogOffset.x, m_dialogOffset.y, size.x, size.y});
}

void InfoDialog::remove_from_viewport()
{
   m_rootBox.remove_from_viewport();
}

void InfoDialog::on_child_state_changed(IWidget& /*widget*/)
{
   this->add_to_viewport({});
}

void InfoDialog::on_event(const ui_core::Event& event)
{
   const auto size = this->desired_size({600, 1200});
   const auto position = event.mousePosition - m_dialogOffset;
   if (position.x > size.x || position.y > size.y) {
      return;
   }

   ui_core::Event subEvent{event};
   subEvent.parentSize = {600, 1200};
   subEvent.mousePosition = position;

   m_rootBox.on_event(subEvent);

   if (event.eventType == ui_core::Event::Type::MouseReleased) {
      if (m_isDragging) {
         m_isDragging = false;
         m_dragOffset.reset();
         m_surface.set_cursor_icon(desktop::CursorIcon::Arrow);
      }
   } else if (event.eventType == ui_core::Event::Type::MouseMoved) {
      if (!m_isDragging)
         return;

      if (!m_dragOffset.has_value()) {
         m_dragOffset.emplace(m_dialogOffset - event.mousePosition);
      }

      m_dialogOffset = event.mousePosition + *m_dragOffset;
      this->add_to_viewport({});
   }
}

void InfoDialog::set_fps(const float value) const
{
   const auto framerateStr = format("{:.1f}", value);
   m_values.at("metrics.fps"_name)->set_content(framerateStr.view());
}

void InfoDialog::set_min_fps(const float value) const
{
   const auto framerateStr = format("{:.1f}", value);
   m_values.at("metrics.fps_min"_name)->set_content(framerateStr.view());
}

void InfoDialog::set_max_fps(const float value) const
{
   const auto framerateMaxStr = format("{:.1f}", value);
   m_values.at("metrics.fps_max"_name)->set_content(framerateMaxStr.view());
}

void InfoDialog::set_avg_fps(const float value) const
{
   const auto framerateAvgStr = format("{:.1f}", value);
   m_values.at("metrics.fps_avg"_name)->set_content(framerateAvgStr.view());
}

void InfoDialog::set_gpu_time(const float value) const
{
   const auto gBufferGpuTimeStr = format("{:.2f}ms", value);
   m_values.at("metrics.gpu_time"_name)->set_content(gBufferGpuTimeStr.view());
}

void InfoDialog::set_triangle_count(const u32 value) const
{
   const auto primitiveCountStr = format("{}", value);
   m_values.at("metrics.triangles"_name)->set_content(primitiveCountStr.view());
}

void InfoDialog::set_camera_pos(const Vector3 value) const
{
   const auto positionStr = format("{:.2f}, {:.2f}, {:.2f}", value.x, value.y, value.z);
   m_values.at("location.position"_name)->set_content(positionStr.view());
}

void InfoDialog::set_orientation(const Vector2 value) const
{
   const auto orientationStr = format("{:.2f}, {:.2f}", value.x, value.y);
   m_values.at("location.orientation"_name)->set_content(orientationStr.view());
}

void InfoDialog::on_config_property_changed(const ConfigProperty property, const Config& config) const
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
      m_values.at("features.bloom"_name)->set_content(config.isBloomEnabled ? "On"_strv : "Off"_strv);
      break;
   case ConfigProperty::IsSmoothCameraEnabled:
      m_values.at("features.smooth_camera"_name)->set_content(config.isSmoothCameraEnabled ? "On"_strv : "Off"_strv);
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
   m_values.at("features.bloom"_name)->set_content(config.isBloomEnabled ? "On"_strv : "Off"_strv);
   m_values.at("features.smooth_camera"_name)->set_content(config.isSmoothCameraEnabled ? "On"_strv : "Off"_strv);
}

void InfoDialog::on_title_clicked(desktop::MouseButton /*button*/)
{
   m_isDragging = true;
   m_surface.set_cursor_icon(desktop::CursorIcon::Move);
}

void InfoDialog::on_title_enter() const
{
   m_title->set_color({0.9, 0.9, 0.5, 1});
}

void InfoDialog::on_title_leave() const
{
   m_title->set_color({1, 1, 1, 1});
}

}// namespace triglav::renderer
