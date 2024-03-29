#include "triglav/desktop/IDisplay.hpp"
#include "triglav/desktop/Entrypoint.hpp"

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;

int triglav_main(InputArgs& args, IDisplay& display);

int main(const int argc, const char** argv)
{
   InputArgs inputArgs{
      .args = argv,
      .arg_count = argc,
   };

   const auto display = triglav::desktop::get_display();
   return triglav_main(inputArgs, *display);
}
