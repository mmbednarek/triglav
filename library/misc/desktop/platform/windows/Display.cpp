#include "Display.hpp"

#include "Surface.hpp"

namespace triglav::desktop {

namespace {

LRESULT CALLBACK process_messages(const HWND window_handle, const UINT msg, const WPARAM w_param, const LPARAM l_param)
{
   if (msg == WM_CREATE) {
      const auto* create_struct = reinterpret_cast<CREATESTRUCT*>(l_param);
      SetWindowLongPtrW(window_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create_struct->lp_create_params));
      return 0;
   }

   auto* surface = reinterpret_cast<Surface*>(GetWindowLongPtrW(window_handle, GWLP_USERDATA));
   if (surface != nullptr) {
      return surface->handle_window_event(msg, w_param, l_param);
   }
   return DefWindowProcW(window_handle, msg, w_param, l_param);
}

}// namespace

Display::Display(const HINSTANCE instance) :
    m_instance(instance)
{
   WNDCLASSEXW wc;
   wc.cb_size = sizeof(WNDCLASSEXW);
   wc.style = 0;
   wc.lpfn_wnd_proc = process_messages;
   wc.cb_cls_extra = 0;
   wc.cb_wnd_extra = 0;
   wc.h_instance = m_instance;
   wc.h_icon = LoadIcon(nullptr, IDI_APPLICATION);
   wc.h_cursor = LoadCursor(nullptr, IDC_ARROW);
   wc.hbr_background = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
   wc.lpsz_menu_name = nullptr;
   wc.lpsz_class_name = g_window_class_name;
   wc.h_icon_sm = LoadIcon(nullptr, IDI_APPLICATION);

   if (not RegisterClassExW(&wc)) {
      MessageBoxW(nullptr, L"Cannot register window class", L"Error", MB_ICONEXCLAMATION | MB_OK);
   }
}

void Display::dispatch_messages()
{
   MSG message;
   if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
   }
}

std::shared_ptr<ISurface> Display::create_surface(const StringView title, const Vector2u dimensions, const WindowAttributeFlags flags)
{
   return std::make_shared<Surface>(*this, title, dimensions, flags, false);
}

std::unique_ptr<IDisplay> get_display()
{
   return std::make_unique<Display>(GetModuleHandleW(nullptr));
}

HINSTANCE Display::winapi_instance() const
{
   return m_instance;
}

}// namespace triglav::desktop
