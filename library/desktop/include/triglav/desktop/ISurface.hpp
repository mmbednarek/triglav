#pragma once

#include "ISurfaceEventListener.hpp"

#include "triglav/EnumFlags.hpp"
#include "triglav/Int.hpp"

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

struct Dimension
{
   int width;
   int height;
};

class ISurface : public std::enable_shared_from_this<ISurface>
{
 public:
   virtual ~ISurface() = default;

   virtual void lock_cursor() = 0;
   virtual void unlock_cursor() = 0;
   virtual void hide_cursor() const = 0;
   virtual void add_event_listener(ISurfaceEventListener* eventListener) = 0;

   [[nodiscard]] virtual bool is_cursor_locked() const = 0;
   [[nodiscard]] virtual Dimension dimension() const = 0;
};

}// namespace triglav::desktop