#pragma once

#include <cstdint>

namespace triglav::desktop {

class Surface;
using MouseButton = uint32_t;

constexpr MouseButton MouseButton_Left = 272;
constexpr MouseButton MouseButton_Right = 273;
constexpr MouseButton MouseButton_Middle = 274;

class ISurfaceEventListener
{
 public:
   virtual ~ISurfaceEventListener() = default;

   virtual void on_mouse_enter(float pos_x, float pos_y)        = 0;
   virtual void on_mouse_move(float pos_x, float pos_y)         = 0;
   virtual void on_mouse_relative_move(float dx, float dy)      = 0;
   virtual void on_mouse_leave()                                = 0;
   virtual void on_mouse_wheel_turn(float x)                    = 0;
   virtual void on_mouse_button_is_pressed(MouseButton button)  = 0;
   virtual void on_mouse_button_is_released(MouseButton button) = 0;
   virtual void on_resize(int width, int height)                = 0;
   virtual void on_close()                                      = 0;
   virtual void on_key_is_pressed(uint32_t key)                 = 0;
   virtual void on_key_is_released(uint32_t key)                = 0;
};

class DefaultSurfaceEventListener : public ISurfaceEventListener
{
 public:
   void on_mouse_enter(float pos_x, float pos_y) override {}

   void on_mouse_move(float pos_x, float pos_y) override {}

   void on_mouse_relative_move(float dx, float dy) override {}

   void on_mouse_leave() override {}

   void on_mouse_wheel_turn(float x) override {}

   void on_mouse_button_is_pressed(MouseButton button) override {}

   void on_mouse_button_is_released(MouseButton button) override {}

   void on_resize(int width, int height) override {}

   void on_close() override {}

   void on_key_is_pressed(uint32_t key) override {}

   void on_key_is_released(uint32_t key) override {}
};

}// namespace wayland