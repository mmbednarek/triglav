#pragma once

#include "triglav/Delegate.hpp"

namespace triglav::desktop::x11 {

class Mouse {
 public:
   using OnMouseMoveDel = Delegate<float, float>;
   OnMouseMoveDel OnMouseMove;

   Mouse();

   void tick();

 private:
   int m_fileDescriptor;
};

}