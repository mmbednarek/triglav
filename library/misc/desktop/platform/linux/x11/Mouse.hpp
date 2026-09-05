#pragma once

#include "triglav/Event.hpp"

namespace triglav::desktop::x11 {

class Mouse
{
 public:
   TG_TAG_CLASS(triglav::desktop::x11::Mouse)

   TG_EVENT(OnMouseMove, float, float)

   Mouse();

   void tick();

 private:
   int m_file_descriptor;
};

}// namespace triglav::desktop::x11