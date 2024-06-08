#pragma once

#include "triglav/desktop/ISurfaceEventListener.hpp"

#include <memory>

namespace triglav::desktop {

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