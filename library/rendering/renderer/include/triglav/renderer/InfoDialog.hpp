#pragma once

#include "Config.hpp"

#include "triglav/render_core/GlyphAtlas.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/widget/Button.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"

#include <map>

namespace triglav::render_core {
class GlyphCache;
}

namespace triglav::desktop {
class ISurface;
}

namespace triglav::renderer {

class InfoDialog final : public ui_core::IWidget
{
 public:
   using Self = InfoDialog;

   InfoDialog(ui_core::Context& context, ConfigManager& configManager, desktop::ISurface& surface);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_child_state_changed(IWidget& widget) override;
   void on_event(const ui_core::Event& event) override;

   void set_fps(float value) const;
   void set_min_fps(float value) const;
   void set_max_fps(float value) const;
   void set_avg_fps(float value) const;
   void set_gpu_time(float value) const;
   void set_triangle_count(u32 value) const;
   void set_camera_pos(Vector3 value) const;
   void set_orientation(Vector2 value) const;

   void on_config_property_changed(ConfigProperty property, const Config& config) const;
   void init_config_labels() const;

   void on_title_clicked(desktop::MouseButton button);
   void on_title_enter() const;
   void on_title_leave() const;

 private:
   ConfigManager& m_configManager;
   desktop::ISurface& m_surface;
   ui_core::RectBox m_rootBox;
   std::map<Name, ui_core::TextBox*> m_values;
   bool m_isDragging{false};
   Vector2 m_dialogOffset{20, 20};
   Vector4 m_croppingMask;
   std::optional<Vector2> m_dragOffset;
   ui_core::TextBox* m_title;

   TG_SINK(ConfigManager, OnPropertyChanged);
   TG_OPT_SINK(ui_core::Button, OnClick);
   TG_OPT_SINK(ui_core::Button, OnEnter);
   TG_OPT_SINK(ui_core::Button, OnLeave);
};

}// namespace triglav::renderer