#include "Display.h"

#include "Surface.h"

namespace triglav::desktop {
namespace {

LRESULT CALLBACK process_messages(const HWND windowHandle, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
   if (msg == WM_CREATE) {
      const auto* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
      SetWindowLongPtrA(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
      return 0;
   }

   auto* surface = reinterpret_cast<Surface*>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
   if (surface != nullptr) {
      return surface->handle_window_event(msg, wParam, lParam);
   }
   return DefWindowProcA(windowHandle, msg, wParam, lParam);
}

}// namespace

Display::Display(const HINSTANCE instance) :
    m_instance(instance)
{
   WNDCLASSEX wc;
   wc.cbSize = sizeof(WNDCLASSEX);
   wc.style = 0;
   wc.lpfnWndProc = process_messages;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = m_instance;
   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
   wc.lpszMenuName = nullptr;
   wc.lpszClassName = g_windowClassName;
   wc.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);

   if (not RegisterClassExA(&wc)) {
      MessageBox(nullptr, "Cannot register window class", "Error", MB_ICONEXCLAMATION | MB_OK);
   }
}

void Display::dispatch_messages() const
{
   MSG message;
   if (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE) > 0) {
      TranslateMessage(&message);
      DispatchMessageA(&message);
   }
}

std::shared_ptr<ISurface> Display::create_surface(const int width, const int height, WindowAttributeFlags flags)
{
   return std::make_shared<Surface>(m_instance, Dimension{width, height}, flags);
}

std::unique_ptr<IDisplay> get_display()
{
   return std::make_unique<Display>(GetModuleHandleA(nullptr));
}

}// namespace triglav::desktop
