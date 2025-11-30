#pragma once

#include "triglav/EnumFlags.hpp"
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

   Digit1,
   Digit2,
   Digit3,
   Digit4,
   Digit5,
   Digit6,
   Digit7,
   Digit8,
   Digit9,
   Digit0,

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
   Enter,
   Delete,

   LeftBrace,
   RightBrace,
   Semicolon,
   Quote,
   Tilde,
   Comma,
   Dot,
   Slash,

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

enum class Modifier : u32
{
   Empty = 0,
   Control = (1 << 0u),
   Shift = (1 << 1u),
   Alt = (1 << 2u),
};

TRIGLAV_DECL_FLAGS(Modifier)

}// namespace triglav::desktop