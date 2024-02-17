#include "Display.h"

#include "ISurface.hpp"
#include "Surface.h"

#include <iostream>

namespace triglav::desktop::x11 {

Display::Display() :
    m_display(XOpenDisplay(nullptr)),
    m_rootWindow(DefaultRootWindow(m_display))
{
}

Display::~Display()
{
   XCloseDisplay(m_display);
}

void Display::dispatch_messages() const
{
   int limit = 32;

   XEvent event;
   while (XPending(m_display) > 0) {
      XNextEvent(m_display, &event);
      if (XFilterEvent(&event, None))
         continue;

      switch (event.type) {
      case KeyPress: {
         auto *surface = dynamic_cast<Surface *>(m_surfaces.at(event.xkey.window).get());
         surface->dispatch_key_press(event.xkey.keycode);
         break;
      }
      case KeyRelease: {
         auto *surface = dynamic_cast<Surface *>(m_surfaces.at(event.xkey.window).get());
         surface->dispatch_key_release(event.xkey.keycode);
         break;
      }
      case ButtonPress: {
         auto *surface = dynamic_cast<Surface *>(m_surfaces.at(event.xbutton.window).get());
         surface->dispatch_button_press(event.xbutton.button);
         break;
      }
      case ButtonRelease: {
         auto *surface = dynamic_cast<Surface *>(m_surfaces.at(event.xbutton.window).get());
         surface->dispatch_button_release(event.xbutton.button);
         break;
      }
      case MotionNotify: {
         auto *surface = dynamic_cast<Surface *>(m_surfaces.at(event.xmotion.window).get());
         surface->dispatch_mouse_move(event.xmotion.x, event.xmotion.y);
         break;
      }
      }
   }

   for (const auto &[window, surfaceIFace] : m_surfaces) {
      const auto *surface = dynamic_cast<Surface *>(surfaceIFace.get());
      surface->tick();
   }
}

std::shared_ptr<ISurface> Display::create_surface(const int width, const int height)
{
   auto window = XCreateSimpleWindow(m_display, m_rootWindow, 0, 0, width, height, 0, 0, 0xffffffff);
   XMapWindow(m_display, window);
   XSelectInput(m_display, window,
                ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                        PointerMotionMask | EnterNotify | LeaveNotify);

   auto surface = std::make_shared<Surface>(m_display, window, Dimension{width, height});
   m_surfaces.emplace(window, surface);
   return surface;
}

}// namespace triglav::desktop::x11

namespace triglav::desktop {

std::unique_ptr<IDisplay> get_display()
{
   return std::make_unique<x11::Display>();
}

}// namespace triglav::desktop
