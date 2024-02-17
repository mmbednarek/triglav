#pragma once

#define TRIGLAV_DESKTOP_WAYLAND 1

#if TRIGLAV_DESKTOP_WAYLAND
struct wl_display;
struct wl_surface;
#endif


namespace triglav::desktop {

#if TRIGLAV_DESKTOP_WAYLAND
struct WaylandSurface
{
   wl_display *display;
   wl_surface *surface;
};
#endif

#if TRIGLAV_DESKTOP_WINDOWS
struct WindowsSurface
{
};
#endif

}