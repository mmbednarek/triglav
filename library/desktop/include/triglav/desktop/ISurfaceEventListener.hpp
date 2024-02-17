#pragma once

#include <cstdint>

namespace triglav::desktop {

class Surface;
// using MouseButton = uint32_t;

constexpr int MouseButton_Left   = 272;
constexpr int MouseButton_Right  = 273;
constexpr int MouseButton_Middle = 274;

enum class MouseButton
{
   Left,
   Right,
   Middle,
   Unknown,
};

enum class Key
{
   Q,
   W,
   E,
   R,
   T,
   Y,
   U,
   I,
   O,
   P,
   A,
   S,
   D,
   F,
   G,
   H,
   J,
   K,
   L,
   Z,
   X,
   C,
   V,
   B,
   N,
   M,

   F1,
   F2,
   F3,
   F4,
   F5,
   F6,
   F7,
   F8,
   F9,
   F10,
   F11,
   F12,

   Space,
   Unknown,
};

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
   virtual void on_key_is_pressed(Key key)                 = 0;
   virtual void on_key_is_released(Key key)                = 0;
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

   void on_key_is_pressed(Key key) override {}

   void on_key_is_released(Key key) override {}
};

}// namespace triglav::desktop