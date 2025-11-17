#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;

int triglav_main(InputArgs& args, IDisplay& display);

int main(const int argc, const char** argv)
{
   InputArgs input_args{
      .args = argv,
      .arg_count = argc,
   };

   const auto display = triglav::desktop::get_display();
   return triglav_main(input_args, *display);
}
