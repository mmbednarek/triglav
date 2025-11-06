#pragma once

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
};

}// namespace triglav::editor