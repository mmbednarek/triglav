#include "Display.hpp"

#include "ISurface.hpp"
#include "Surface.hpp"

#include <X11/Xatom.h>

#include <iostream>
#include <ranges>

namespace triglav::desktop::x11 {

Display::Display() :
    m_display(XOpenDisplay(nullptr)),
    m_rootWindow(DefaultRootWindow(m_display)),
    m_wmDeleteAtom(XInternAtom(m_display, "WM_DELETE_WINDOW", true)),
    TG_CONNECT(m_mouse, OnMouseMove, on_mouse_move)
{
}

Display::~Display()
{
   XCloseDisplay(m_display);
}

void Display::dispatch_messages()
{
   XEvent event;
   while (XPending(m_display) > 0) {
      XNextEvent(m_display, &event);
      if (XFilterEvent(&event, 0L))
         continue;

      switch (event.type) {
      case KeyPress: {
         auto surface = this->surface_by_window(event.xkey.window);
         if (surface) {
            surface->dispatch_key_press(event);
         }
         break;
      }
      case KeyRelease: {
         auto surface = this->surface_by_window(event.xkey.window);
         if (surface) {
            surface->dispatch_key_release(event);
         }
         break;
      }
      case ButtonPress: {
         auto surface = this->surface_by_window(event.xbutton.window);
         if (surface) {
            surface->dispatch_button_press(event);
         }
         break;
      }
      case ButtonRelease: {
         auto surface = this->surface_by_window(event.xbutton.window);
         if (surface) {
            surface->dispatch_button_release(event);
         }
         break;
      }
      case MotionNotify: {
         auto surface = this->surface_by_window(event.xmotion.window);
         if (surface) {
            surface->dispatch_mouse_move(event);
         }
         break;
      }
      case ClientMessage: {
         if (event.xclient.data.l[0] == static_cast<int>(m_wmDeleteAtom)) {
            auto surface = this->surface_by_window(event.xclient.window);
            if (surface) {
               surface->dispatch_close();
            }
         }
         break;
      }
      case ConfigureNotify: {
         auto surface = this->surface_by_window(event.xconfigure.window);
         if (surface) {
            surface->dispatch_resize({event.xconfigure.width, event.xconfigure.height});
         }
         break;
      }
      }
   }

   for (const auto wndHandle : std::views::keys(m_surfaces)) {
      auto surface = this->surface_by_window(wndHandle);
      if (not surface)
         break;
      surface->tick();
   }

   m_mouse.tick();
}

std::shared_ptr<ISurface> Display::create_surface(const StringView title, const Vector2u dimensions, const WindowAttributeFlags flags)
{
   auto window = XCreateSimpleWindow(m_display, m_rootWindow, 0, 0, dimensions.x, dimensions.y, 0, 0, 0xffffffff);

   XStoreName(m_display, window, title.data());

   if (not(flags & WindowAttribute::ShowDecorations)) {
      Atom window_type = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE", False);
      auto value = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE_DOCK", False);
      XChangeProperty(m_display, window, window_type, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char*>(&value), 1);
   }

   XMapWindow(m_display, window);
   XSelectInput(m_display, window,
                ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterNotify |
                   LeaveNotify | StructureNotifyMask);
   XSetWMProtocols(m_display, window, &m_wmDeleteAtom, 1);

   auto surface = std::make_shared<Surface>(*this, window, dimensions);
   this->map_surface(window, surface->weak_from_this());
   return surface;
}

void Display::map_surface(const ::Window window, const std::weak_ptr<ISurface>& surface)
{
   m_surfaces.emplace(window, surface);
}

void Display::on_mouse_move(const float x, const float y)
{
   for (const auto wndHandle : std::views::keys(m_surfaces)) {
      auto surface = this->surface_by_window(wndHandle);
      if (not surface)
         break;

      surface->dispatch_mouse_relative_move({1.5f * x, 1.5f * y});
   }
}

std::shared_ptr<Surface> Display::surface_by_window(const Window wndHandle)
{
   auto it = m_surfaces.find(wndHandle);
   if (it == m_surfaces.end())
      return nullptr;

   auto surface = it->second.lock();
   if (not surface) {
      m_surfaces.erase(wndHandle);
      return nullptr;
   }

   return std::dynamic_pointer_cast<Surface>(surface);
}

}// namespace triglav::desktop::x11
