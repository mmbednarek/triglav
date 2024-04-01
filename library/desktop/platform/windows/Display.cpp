#include "Display.h"

#include "Surface.h"

#include <Windowsx.h>

namespace triglav::desktop {namespace {

   LRESULT CALLBACK process_messages(const HWND windowHandle, const UINT msg, const WPARAM wParam,
                                     const LPARAM lParam)
   {
      switch (msg) {
      case WM_CREATE: {
         const auto *createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
         SetWindowLongPtrA(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
         break;
      }
      case WM_CLOSE: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_close();
         break;
      }
      case WM_DESTROY:
         PostQuitMessage(0);
         break;
      case WM_KEYDOWN: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_key_down(wParam);
         break;
      }
      case WM_KEYUP: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_key_up(wParam);
         break;
      }
      case WM_LBUTTONDOWN: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_button_down(MouseButton::Left);
         break;
      }
      case WM_MBUTTONDOWN: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_button_down(MouseButton::Middle);
         break;
      }
      case WM_RBUTTONDOWN: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_button_down(MouseButton::Right);
         break;
      }
      case WM_LBUTTONUP: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_button_up(MouseButton::Left);
         break;
      }
      case WM_MBUTTONUP: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_button_up(MouseButton::Middle);
         break;
      }
      case WM_RBUTTONUP: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_button_up(MouseButton::Right);
         break;
      }
      case WM_MOUSEMOVE: {
         auto *surface = reinterpret_cast<Surface *>(GetWindowLongPtrA(windowHandle, GWLP_USERDATA));
         surface->on_mouse_move(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
         break;
      }
      default: return DefWindowProcA(windowHandle, msg, wParam, lParam);
      }
      return 0;
   }

   }

   Display::Display(const HINSTANCE instance) :
      m_instance(instance)
   {
      WNDCLASSEX wc;
      wc.cbSize        = sizeof(WNDCLASSEX);
      wc.style         = 0;
      wc.lpfnWndProc   = process_messages;
      wc.cbClsExtra    = 0;
      wc.cbWndExtra    = 0;
      wc.hInstance     = m_instance;
      wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
      wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
      wc.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));
      wc.lpszMenuName  = nullptr;
      wc.lpszClassName = g_windowClassName;
      wc.hIconSm       = LoadIconA(nullptr, IDI_APPLICATION);

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

   std::shared_ptr<ISurface> Display::create_surface(const int width, const int height)
   {
      return std::make_shared<Surface>(m_instance, Dimension{width, height});
   }

   std::unique_ptr<IDisplay> get_display()
   {
      return std::make_unique<Display>(GetModuleHandleA(nullptr));
   }

}
