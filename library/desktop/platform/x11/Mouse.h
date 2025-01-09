#pragma once

#include "triglav/event/Delegate.hpp"

namespace triglav::desktop::x11 {

class Mouse
{
 public:
   TG_EVENT(OnMouseMove, float, float);

   Mouse();

   void tick();

 private:
   int m_fileDescriptor;
};

}// namespace triglav::desktop::x11