#pragma once

#include "triglav/Int.hpp"

namespace triglav::desktop {

constexpr i32 MouseButton_Left = 272;
constexpr i32 MouseButton_Right = 273;
constexpr i32 MouseButton_Middle = 274;

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
   Backspace,
   Tab,
   Escape,
   Control,
   Shift,
   Alt,

   LeftArrow,
   UpArrow,
   RightArrow,
   DownArrow,
};

enum class CursorIcon
{
   Arrow,
   Hand,
   Move,
   Wait,
   Edit,
   ResizeHorizontal,
   ResizeVertical,
};

}// namespace triglav::desktop