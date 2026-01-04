#pragma once

#include "Desktop.hpp"

#include "triglav/EnumFlags.hpp"
#include "triglav/Int.hpp"
#include "triglav/Math.hpp"
#include "triglav/Utf8.hpp"
#include "triglav/event/Delegate.hpp"

#include <memory>

#ifdef None
#define TRI_TMP_None None
#undef None
#endif

namespace triglav::desktop {

enum class WindowAttribute : u32
{
   None = 0,
   ShowDecorations = (1 << 0),
   AlignCenter = (1 << 1),
   TopMost = (1 << 2),
   Resizeable = (1 << 3),

   Default = ShowDecorations | Resizeable,
};

TRIGLAV_DECL_FLAGS(WindowAttribute)

enum class KeyboardInputMode
{
   None = 0,
   Direct = (1 << 0),
   Text = (1 << 1),
};

TRIGLAV_DECL_FLAGS(KeyboardInputMode)


using Dimension = Vector2i;

class ISurface : public std::enable_shared_from_this<ISurface>
{
 public:
   TG_EVENT(OnMouseEnter, Vector2)
   TG_EVENT(OnMouseMove, Vector2)
   TG_EVENT(OnMouseRelativeMove, Vector2)
   TG_EVENT(OnMouseLeave)
   TG_EVENT(OnMouseWheelTurn, float)
   TG_EVENT(OnMouseButtonIsPressed, MouseButton)
   TG_EVENT(OnMouseButtonIsReleased, MouseButton)
   TG_EVENT(OnMouseDoubleClick, MouseButton)
   TG_EVENT(OnResize, Vector2i)
   TG_EVENT(OnClose)
   TG_EVENT(OnKeyIsPressed, Key)
   TG_EVENT(OnKeyIsReleased, Key)
   TG_EVENT(OnTextInput, Rune)

   ISurface() = default;
   virtual ~ISurface() = default;

   ISurface(const ISurface& surface) = delete;
   ISurface& operator=(const ISurface& surface) = delete;
   ISurface(ISurface&& surface) noexcept = delete;
   ISurface& operator=(ISurface&& surface) noexcept = delete;

   virtual void lock_cursor() = 0;
   virtual void unlock_cursor() = 0;
   virtual void hide_cursor() const = 0;
   virtual void set_cursor_icon(CursorIcon icon) = 0;
   virtual void set_keyboard_input_mode(KeyboardInputModeFlags mode) = 0;
   virtual std::shared_ptr<ISurface> create_popup(Vector2u dimensions, Vector2 offset, WindowAttributeFlags flags) = 0;
   virtual ModifierFlags modifiers() const = 0;

   [[nodiscard]] virtual bool is_cursor_locked() const = 0;
   [[nodiscard]] virtual Vector2i dimension() const = 0;
};

}// namespace triglav::desktop