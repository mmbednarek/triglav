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

namespace triglav::renderer {

class InfoDialog final : public ui_core::IWidget
{
 public:
   using Self = InfoDialog;

   InfoDialog(ui_core::Context& context, ConfigManager& configManager);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;
   void on_child_state_changed(IWidget& widget) override;
   void on_mouse_click(desktop::MouseButton mouseButton, Vector2 parentSize, Vector2 position) override;
   void on_mouse_is_released(desktop::MouseButton mouseButton, Vector2 position);

   void set_fps(float value) const;
   void set_min_fps(float value) const;
   void set_max_fps(float value) const;
   void set_avg_fps(float value) const;
   void set_gpu_time(float value) const;
   void set_triangle_count(u32 value) const;
   void set_camera_pos(Vector3 value) const;
   void set_orientation(Vector2 value) const;

   void on_config_property_changed(ConfigProperty property, const Config& config);
   void init_config_labels() const;

   void on_title_clicked(desktop::MouseButton button);
   void on_mouse_move(Vector2 position);

 private:
   ui_core::Context& m_context;
   ConfigManager& m_configManager;
   ui_core::RectBox m_rootBox;
   std::map<Name, ui_core::TextBox*> m_values;
   bool m_isDragging{false};
   Vector2 m_dialogOffset{20, 20};
   std::optional<Vector2> m_dragOffset;

   TG_SINK(ConfigManager, OnPropertyChanged);
   TG_OPT_SINK(ui_core::Button, OnClick);
};

}// namespace triglav::renderer