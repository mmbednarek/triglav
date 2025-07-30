#include "Mouse.hpp"

#include <array>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

namespace triglav::desktop::x11 {

Mouse::Mouse() :
    m_fileDescriptor(::open("/dev/input/mice", O_RDONLY | O_NONBLOCK))
{
}

void Mouse::tick()
{
   std::array<char, 256> buffer{};

   for (;;) {
      auto result = ::read(m_fileDescriptor, buffer.data(), 256);
      if (result < 3)
         return;

      for (int i = 0; i < result; i += 3) {
         event_OnMouseMove.publish(static_cast<float>(buffer[i + 1]), -static_cast<float>(buffer[i + 2]));
      }
   }
}

}// namespace triglav::desktop::x11
