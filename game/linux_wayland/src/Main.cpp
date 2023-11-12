#define _POSIX_C_SOURCE 200112L
#include "xdg-shell-client-protocol.h"
#include <cerrno>
#include <climits>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "renderer/Renderer.h"

/* Shared memory support code */
static void randname(char *buf)
{
   timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);
   long r = ts.tv_nsec;
   for (int i = 0; i < 6; ++i) {
      buf[i] = 'A' + (r & 15) + (r & 16) * 2;
      r >>= 5;
   }
}

static int create_shm_file(void)
{
   int retries = 100;
   do {
      char name[] = "/wl_shm-XXXXXX";
      randname(name + sizeof(name) - 7);
      --retries;
      int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
      if (fd >= 0) {
         shm_unlink(name);
         return fd;
      }
   } while (retries > 0 && errno == EEXIST);
   return -1;
}

static int allocate_shm_file(size_t size)
{
   int fd = create_shm_file();
   if (fd < 0)
      return -1;
   int ret;
   do {
      ret = ftruncate(fd, size);
   } while (ret < 0 && errno == EINTR);
   if (ret < 0) {
      close(fd);
      return -1;
   }
   return fd;
}

/* Wayland code */
struct client_state
{
   /* Globals */
   struct wl_display *wl_display;
   struct wl_registry *wl_registry;
   struct wl_shm *wl_shm;
   struct wl_compositor *wl_compositor;
   struct xdg_wm_base *xdg_wm_base;
   /* Objects */
   struct wl_surface *wl_surface;
   struct xdg_surface *xdg_surface;
   struct xdg_toplevel *xdg_toplevel;
};

static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
   /* Sent by the compositor when it's no longer using this buffer */
   wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
        .release = wl_buffer_release,
};

int g_width = 1920;
int g_height = 1080;

bool g_should_resize = false;
bool g_resize_requested = false;
int g_new_width{};
int g_new_height{};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
   xdg_surface_ack_configure(xdg_surface, serial);

   if (g_resize_requested)
   {
      g_should_resize = true;
   }
}

static const struct xdg_surface_listener xdg_surface_listener = {
        .configure = xdg_surface_configure,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
   xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
        .ping = xdg_wm_base_ping,
};

static void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface,
                            uint32_t version)
{
   struct client_state *state = static_cast<client_state *>(data);
   if (strcmp(interface, wl_shm_interface.name) == 0) {
      state->wl_shm = static_cast<wl_shm *>(wl_registry_bind(wl_registry, name, &wl_shm_interface, 1));
   } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
      state->wl_compositor =
              static_cast<wl_compositor *>(wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4));
   } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
      state->xdg_wm_base =
              static_cast<xdg_wm_base *>(wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1));
      xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
   }
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name)
{
   /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
        .global        = registry_global,
        .global_remove = registry_global_remove,
};

static void handleToplevelConfigure(
        void* data,
        struct xdg_toplevel* toplevel,
        int32_t width,
        int32_t height,
        struct wl_array* states
)
{
   if (width != 0 && height != 0)
   {
      std::cerr << "requested resize: " << width << ", " << height << '\n';
      g_resize_requested = true;
      g_new_width = width;
      g_new_height = height;
   }
}

bool g_should_quit = false;

static void handleToplevelClose(void* data, struct xdg_toplevel* toplevel)
{
   g_should_quit = true;
}

static const struct xdg_toplevel_listener toplevelListener = {
        .configure = handleToplevelConfigure,
        .close = handleToplevelClose
};

int main()
{
   client_state state{};

   state.wl_display          = wl_display_connect(nullptr);
   state.wl_registry         = wl_display_get_registry(state.wl_display);
   wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
   wl_display_roundtrip(state.wl_display);

   state.wl_surface  = wl_compositor_create_surface(state.wl_compositor);
   state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);
   xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
   state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
   xdg_toplevel_add_listener(state.xdg_toplevel, &toplevelListener, nullptr);
   xdg_toplevel_set_title(state.xdg_toplevel, "Example client");
   xdg_toplevel_set_app_id(state.xdg_toplevel, "example-client");
   wl_surface_commit(state.wl_surface);

   graphics_api::Surface surface{
           .display = state.wl_display,
           .surface = state.wl_surface,
   };

   auto renderer = renderer::init_renderer(surface, g_width, g_height);

   bool is_running{true};

   while (is_running) {
      if (g_should_resize) {
         g_should_resize = false;
         g_resize_requested = false;
         g_width = g_new_width;
         g_height = g_new_height;

         renderer.on_resize(g_width, g_height);
      }

      if (g_should_quit) {
         is_running = false;
      }

      renderer.on_render();

      wl_display_dispatch(state.wl_display);
   }

   renderer.on_close();

   return 0;
}
