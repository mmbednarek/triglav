#pragma once

#include "triglav/desktop/Desktop.hpp"
#include "triglav/geometry/Geometry.hpp"

namespace triglav::editor {

class ILevelEditorTool
{
 public:
   virtual ~ILevelEditorTool() = default;

   virtual bool on_use_start(const geometry::Ray& ray) = 0;
   virtual void on_mouse_moved(Vector2 position) = 0;
   virtual void on_view_updated() = 0;
   virtual void on_use_end() = 0;
   virtual void on_left_tool() = 0;
   virtual void on_tick(float /*delta_time*/) {};
   virtual void on_modifiers(desktop::ModifierFlags /*mods*/){};
};

}// namespace triglav::editor