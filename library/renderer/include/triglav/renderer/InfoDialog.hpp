#pragma once

#include "Config.hpp"

#include "triglav/render_core/GlyphAtlas.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/ui_core/Viewport.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/TextBox.hpp"

#include <map>

namespace triglav::render_core {
class GlyphCache;
}

namespace triglav::renderer {

class InfoDialog
{
 public:
   using Self = InfoDialog;

   InfoDialog(ui_core::Context& context, ConfigManager& configManager);

   void initialize();

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

 private:
   ui_core::Context& m_context;
   ConfigManager& m_configManager;
   ui_core::RectBox m_rootBox;
   std::map<Name, ui_core::TextBox*> m_values;

   TG_SINK(ConfigManager, OnPropertyChanged);
};

}// namespace triglav::renderer