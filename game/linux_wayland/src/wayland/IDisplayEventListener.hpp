#pragma once

namespace wayland {

class Surface;

class IDisplayEventListener
{
 public:
   virtual ~IDisplayEventListener() = default;

   virtual void on_pointer_enter_surface(Surface &surface, float pos_x, float pos_y) = 0;
   virtual void on_pointer_change_position(float pos_x, float pos_y)                 = 0;
   virtual void on_pointer_relative_motion(float dx, float dy)                       = 0;
   virtual void on_pointer_leave_surface(Surface &surface)                           = 0;
   virtual void on_mouse_wheel_turn(float x)                                         = 0;
   virtual void on_mouse_button_is_pressed(uint32_t button)                                = 0;
};

}// namespace wayland