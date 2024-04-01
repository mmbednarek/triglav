#include <Windows.h>

#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;

int triglav_main(InputArgs &args, IDisplay &display);

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
   InputArgs inputArgs{
           .args      = nullptr,
           .arg_count = 0,
   };

   auto display = triglav::desktop::get_display();
   return triglav_main(inputArgs, *display);
}
