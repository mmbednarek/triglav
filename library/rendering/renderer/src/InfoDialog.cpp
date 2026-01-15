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

namespace triglav::renderer {

using namespace name_literals;
using namespace string_literals;

constexpr std::array g_metrics_labels{
   std::tuple{"metrics.fps"_name, "Framerate"_strv},
   std::tuple{"metrics.fps_min"_name, "Framerate Min"_strv},
   std::tuple{"metrics.fps_max"_name, "Framerate Max"_strv},
   std::tuple{"metrics.fps_avg"_name, "Framerate Avg"_strv},
   std::tuple{"metrics.triangles"_name, "Triangle Count"_strv},
   std::tuple{"metrics.gpu_time"_name, "GPU Render Time"_strv},
};

constexpr std::array g_location_labels{
   std::tuple{"location.position"_name, "Position"_strv},
   std::tuple{"location.orientation"_name, "Orientation"_strv},
};

constexpr std::array g_feature_labels{
   std::tuple{"features.ao"_name, "Ambient Occlusion"_strv},    std::tuple{"features.aa"_name, "Anti-Aliasing"_strv},
   std::tuple{"features.shadows"_name, "Shadows"_strv},         std::tuple{"features.bloom"_name, "Bloom"_strv},
   std::tuple{"features.debug_lines"_name, "Debug Lines"_strv}, std::tuple{"features.smooth_camera"_name, "Smooth Camera"_strv},
};

constexpr std::array g_label_groups{
   std::tuple{"Metrics"_strv, std::span{g_metrics_labels.data(), g_metrics_labels.size()}},
   std::tuple{"Location"_strv, std::span{g_location_labels.data(), g_location_labels.size()}},
   std::tuple{"Features"_strv, std::span{g_feature_labels.data(), g_feature_labels.size()}},
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
       m_vertical_layout(ctx,
                         {
                            .padding = state.padding,
                            .separation = state.separation,
                         },
                         this),
       m_label_box(m_vertical_layout.create_child<ui_core::Button>({})),
       m_label_layout(m_label_box.create_content<ui_core::HorizontalLayout>({
          .separation = 10.0f,
          .gravity = ui_core::HorizontalAlignment::Center,
       })),
       m_image(m_label_layout.create_child<ui_core::Image>({
          .texture = "texture/ui_images.tex"_rc,
          .max_size = Vector2{20, 20},
          .region = Vector4{400, 0, 200, 200},
       })),
       m_label_text(m_label_layout.create_child<ui_core::TextBox>({
          .font_size = 20,
          .typeface = "fonts/segoeui/bold.typeface"_rc,
          .content = state.label,
          .color = {1.0f, 1.0f, 0.4f, 1.0f},
          .vertical_alignment = ui_core::VerticalAlignment::Center,
       })),
       m_content(m_vertical_layout.create_child<ui_core::HideableWidget>({
          .is_hidden = false,
       })),
       TG_CONNECT(m_label_box, OnClick, on_click),
       TG_CONNECT(m_label_box, OnEnter, on_enter),
       TG_CONNECT(m_label_box, OnLeave, on_leave)
   {
   }

   [[nodiscard]] Vector2 desired_size(const Vector2 available_size) const override
   {
      return m_vertical_layout.desired_size(available_size);
   }

   void add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask) override
   {
      m_vertical_layout.add_to_viewport(dimensions, cropping_mask);
   }

   void remove_from_viewport() override
   {
      m_vertical_layout.remove_from_viewport();
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
      return m_vertical_layout.on_event(event);
   }

   void on_click(const desktop::MouseButton /*mouse_button*/) const
   {
      if (m_content.state().is_hidden) {
         m_image.set_region({400, 0, 200, 200});
         m_content.set_is_hidden(false);
      } else {
         m_image.set_region({200, 0, 200, 200});
         m_content.set_is_hidden(true);
      }
   }

   void on_enter() const
   {
      m_label_text.set_color({1.0f, 1.0f, 0.8f, 1.0f});
      if (m_state.surface != nullptr) {
         m_state.surface->set_cursor_icon(desktop::CursorIcon::Hand);
      }
   }

   void on_leave() const
   {
      m_label_text.set_color({1.0f, 1.0f, 0.4f, 1.0f});
      if (m_state.surface != nullptr) {
         m_state.surface->set_cursor_icon(desktop::CursorIcon::Arrow);
      }
   }

 private:
   State m_state;
   ui_core::IWidget* m_parent;
   ui_core::VerticalLayout m_vertical_layout;
   ui_core::Button& m_label_box;
   ui_core::HorizontalLayout& m_label_layout;
   ui_core::Image& m_image;
   ui_core::TextBox& m_label_text;
   ui_core::HideableWidget& m_content;

   TG_SINK(ui_core::Button, OnClick);
   TG_SINK(ui_core::Button, OnEnter);
   TG_SINK(ui_core::Button, OnLeave);
};

InfoDialog::InfoDialog(ui_core::Context& context, ConfigManager& config_manager, desktop::ISurface& surface) :
    m_config_manager(config_manager),
    m_surface(surface),
    m_root_box(context,
               ui_core::RectBox::State{
                  .color = {0.0, 0.0f, 0.0f, 0.75f},
                  .border_radius = {10.0f, 10.0f, 10.0f, 10.0f},
               },
               this),
    TG_CONNECT(m_config_manager, OnPropertyChanged, on_config_property_changed)
{
   auto& viewport = m_root_box.create_content<ui_core::VerticalLayout>({
      .padding = {25.0f, 25.0f, 25.0f, 25.0f},
      .separation = 10.0f,
   });

   auto& title_button = viewport.create_child<ui_core::Button>({});

   auto& title_layout = title_button.create_content<ui_core::HorizontalLayout>({
      .separation = 5.0f,
      .gravity = ui_core::HorizontalAlignment::Center,
   });

   title_layout.create_child<ui_core::Image>({
      .texture = "texture/ui_images.tex"_rc,
      .max_size = Vector2{32, 32},
      .region = Vector4{0, 0, 200, 200},
   });

   m_title = &title_layout.create_child<ui_core::TextBox>({
      .font_size = 24,
      .typeface = "engine/fonts/cantarell/bold.typeface"_rc,
      .content = "Triglav Render Demo",
      .color = {1.0f, 1.0f, 1.0f, 1.0f},
      .horizontal_alignment = ui_core::HorizontalAlignment::Center,
      .vertical_alignment = ui_core::VerticalAlignment::Center,
   });

   TG_CONNECT_OPT(title_button, OnClick, on_title_clicked);
   TG_CONNECT_OPT(title_button, OnEnter, on_title_enter);
   TG_CONNECT_OPT(title_button, OnLeave, on_title_leave);

   viewport.create_child<ui_core::EmptySpace>({
      .size = {1.0f, 4.0f},
   });

   for (const auto& [group_label, group] : g_label_groups) {
      auto& panel = viewport.create_child<HideablePanel>({
         .separation = 8.0f,
         .label = group_label,
         .surface = &m_surface,
      });

      auto& label_box = panel.create_child<ui_core::HorizontalLayout>({
         .padding = {0.0f, 15.0f, 0.0f, 10.0f},
         .separation = 10.0f,
         .gravity = ui_core::HorizontalAlignment::Center,
      });

      auto& left_panel = label_box.create_child<ui_core::VerticalLayout>({
         .padding = {0, 0.0f, 5.0f, 0.0f},
         .separation = 10.0f,
      });

      auto& right_panel = label_box.create_child<ui_core::VerticalLayout>({
         .padding = {5.0f, 0.0f, 0, 0.0f},
         .separation = 10.0f,
      });

      for (const auto& [name, content] : group) {
         left_panel.create_child<ui_core::TextBox>({
            .font_size = 18,
            .typeface = "fonts/segoeui/regular.typeface"_rc,
            .content = content,
            .color = {1.0f, 1.0f, 1.0f, 1.0f},
            .horizontal_alignment = ui_core::HorizontalAlignment::Right,
         });

         m_values[name] = &right_panel.create_child<ui_core::TextBox>({
            .font_size = 18,
            .typeface = "fonts/segoeui/regular.typeface"_rc,
            .content = "none",
            .color = {1.0f, 1.0f, 0.4f, 1.0f},
            .horizontal_alignment = ui_core::HorizontalAlignment::Left,
         });
      }
   }
}

Vector2 InfoDialog::desired_size(const Vector2 available_size) const
{
   return m_root_box.desired_size(available_size);
}

void InfoDialog::add_to_viewport(Vector4 /*dimensions*/, const Vector4 cropping_mask)
{
   const auto size = this->desired_size({600, 1200});
   m_cropping_mask = cropping_mask;
   m_root_box.add_to_viewport({m_dialog_offset.x, m_dialog_offset.y, size.x, size.y}, cropping_mask);
}

void InfoDialog::remove_from_viewport()
{
   m_root_box.remove_from_viewport();
}

void InfoDialog::on_child_state_changed(IWidget& /*widget*/)
{
   this->add_to_viewport({}, m_cropping_mask);
}

void InfoDialog::on_event(const ui_core::Event& event)
{
   const auto size = this->desired_size({600, 1200});
   const auto position = event.mouse_position - m_dialog_offset;
   if (position.x > size.x || position.y > size.y) {
      return;
   }

   ui_core::Event sub_event{event};
   sub_event.widget_size = {600, 1200};
   sub_event.mouse_position = position;

   m_root_box.on_event(sub_event);

   if (event.event_type == ui_core::Event::Type::MouseReleased) {
      if (m_is_dragging) {
         m_is_dragging = false;
         m_drag_offset.reset();
         m_surface.set_cursor_icon(desktop::CursorIcon::Arrow);
      }
   } else if (event.event_type == ui_core::Event::Type::MouseMoved) {
      if (!m_is_dragging)
         return;

      if (!m_drag_offset.has_value()) {
         m_drag_offset.emplace(m_dialog_offset - event.mouse_position);
      }

      m_dialog_offset = event.mouse_position + *m_drag_offset;
      this->add_to_viewport({}, m_cropping_mask);
   }
}

void InfoDialog::set_fps(const float value) const
{
   const auto framerate_str = format("{:.1f}", value);
   m_values.at("metrics.fps"_name)->set_content(framerate_str.view());
}

void InfoDialog::set_min_fps(const float value) const
{
   const auto framerate_str = format("{:.1f}", value);
   m_values.at("metrics.fps_min"_name)->set_content(framerate_str.view());
}

void InfoDialog::set_max_fps(const float value) const
{
   const auto framerate_max_str = format("{:.1f}", value);
   m_values.at("metrics.fps_max"_name)->set_content(framerate_max_str.view());
}

void InfoDialog::set_avg_fps(const float value) const
{
   const auto framerate_avg_str = format("{:.1f}", value);
   m_values.at("metrics.fps_avg"_name)->set_content(framerate_avg_str.view());
}

void InfoDialog::set_gpu_time(const float value) const
{
   const auto g_buffer_gpu_time_str = format("{:.2f}ms", value);
   m_values.at("metrics.gpu_time"_name)->set_content(g_buffer_gpu_time_str.view());
}

void InfoDialog::set_triangle_count(const u32 value) const
{
   const auto primitive_count_str = format("{}", value);
   m_values.at("metrics.triangles"_name)->set_content(primitive_count_str.view());
}

void InfoDialog::set_camera_pos(const Vector3 value) const
{
   const auto position_str = format("{:.2f}, {:.2f}, {:.2f}", value.x, value.y, value.z);
   m_values.at("location.position"_name)->set_content(position_str.view());
}

void InfoDialog::set_orientation(const Vector2 value) const
{
   const auto orientation_str = format("{:.2f}, {:.2f}", value.x, value.y);
   m_values.at("location.orientation"_name)->set_content(orientation_str.view());
}

void InfoDialog::on_config_property_changed(const ConfigProperty property, const Config& config) const
{
   switch (property) {
   case ConfigProperty::AmbientOcclusion:
      m_values.at("features.ao"_name)->set_content(ambient_occlusion_method_to_string(config.ambient_occlusion));
      break;
   case ConfigProperty::Antialiasing:
      m_values.at("features.aa"_name)->set_content(antialiasing_method_to_string(config.antialiasing));
      break;
   case ConfigProperty::ShadowCasting:
      m_values.at("features.shadows"_name)->set_content(shadow_casting_method_to_string(config.shadow_casting));
      break;
   case ConfigProperty::IsBloomEnabled:
      m_values.at("features.bloom"_name)->set_content(config.is_bloom_enabled ? "On"_strv : "Off"_strv);
      break;
   case ConfigProperty::IsSmoothCameraEnabled:
      m_values.at("features.smooth_camera"_name)->set_content(config.is_smooth_camera_enabled ? "On"_strv : "Off"_strv);
      break;
   default:
      break;
   }
}

void InfoDialog::init_config_labels() const
{
   const auto& config = m_config_manager.config();
   m_values.at("features.ao"_name)->set_content(ambient_occlusion_method_to_string(config.ambient_occlusion));
   m_values.at("features.aa"_name)->set_content(antialiasing_method_to_string(config.antialiasing));
   m_values.at("features.shadows"_name)->set_content(shadow_casting_method_to_string(config.shadow_casting));
   m_values.at("features.bloom"_name)->set_content(config.is_bloom_enabled ? "On"_strv : "Off"_strv);
   m_values.at("features.smooth_camera"_name)->set_content(config.is_smooth_camera_enabled ? "On"_strv : "Off"_strv);
}

void InfoDialog::on_title_clicked(desktop::MouseButton /*button*/)
{
   m_is_dragging = true;
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
