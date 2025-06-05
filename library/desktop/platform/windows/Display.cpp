#include "Display.hpp"

#include "Surface.hpp"

namespace triglav::desktop {
namespace {

LRESULT CALLBACK process_messages(const HWND windowHandle, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
   if (msg == WM_CREATE) {
      const auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
      SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
      return 0;
   }

   auto* surface = reinterpret_cast<Surface*>(GetWindowLongPtrW(windowHandle, GWLP_USERDATA));
   if (surface != nullptr) {
      return surface->handle_window_event(msg, wParam, lParam);
   }
   return DefWindowProcW(windowHandle, msg, wParam, lParam);
}

}// namespace

Display::Display(const HINSTANCE instance) :
    m_instance(instance)
{
   WNDCLASSEXW wc;
   wc.cbSize = sizeof(WNDCLASSEXW);
   wc.style = 0;
   wc.lpfnWndProc = process_messages;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = m_instance;
   wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
   wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
   wc.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
   wc.lpszMenuName = nullptr;
   wc.lpszClassName = g_windowClassName;
   wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

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
   return std::make_shared<Surface>(m_instance, title, dimensions, flags);
}

std::unique_ptr<IDisplay> get_display()
{
   return std::make_unique<Display>(GetModuleHandleW(nullptr));
}

}// namespace triglav::desktop
