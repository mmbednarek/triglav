#include "triglav/Logging.hpp"
#include "triglav/desktop/Entrypoint.hpp"
#include "triglav/desktop/IDisplay.hpp"

#include <windows.h>

using triglav::desktop::IDisplay;
using triglav::desktop::InputArgs;

int triglav_main(InputArgs& args, IDisplay& display);

namespace {

InputArgs construct_input_args()
{
   int count{};
   LPWSTR* cmd_args_w = CommandLineToArgvW(GetCommandLineW(), &count);
   if (cmd_args_w == nullptr) {
      return {nullptr, 0};
   }

   int arg_buffer_size = 0;
   for (int i = 0; i < count; ++i) {
      const auto byte_count = WideCharToMultiByte(CP_UTF8, 0, cmd_args_w[i], -1, nullptr, 0, nullptr, nullptr);
      if (byte_count == 0) {
         LocalFree(cmd_args_w);
         return {nullptr, 0};
      }

      arg_buffer_size += byte_count + 1;
   }

   auto* args = new char*[count + 1];
   auto* arg_buffer = new char[arg_buffer_size];

   char* arg = arg_buffer;
   for (int i = 0; i < count; ++i) {
      args[i] = arg;
      const auto bytes_written = WideCharToMultiByte(CP_UTF8, 0, cmd_args_w[i], -1, arg, arg_buffer_size, nullptr, nullptr);
      if (bytes_written == 0) {
         delete[] args;
         delete[] arg_buffer;
         LocalFree(cmd_args_w);
         return {nullptr, 0};
      }

      const auto arg_len = bytes_written + 1;
      arg += arg_len;
      arg_buffer_size -= arg_len;
   }

   args[count] = nullptr;

   LocalFree(cmd_args_w);
   return {const_cast<const char**>(args), count};
}

void enable_virtual_terminal_processing()
{
   HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
   if (handle == INVALID_HANDLE_VALUE) {
      return;
   }

   DWORD dw_mode = 0;
   if (!GetConsoleMode(handle, &dw_mode)) {
      return;
   }

   dw_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

   if (!SetConsoleMode(handle, dw_mode)) {
      return;
   }
}

void create_debug_console()
{
   FILE* conin = stdin;
   FILE* conout = stdout;
   FILE* conerr = stderr;
   AllocConsole();
   AttachConsole(GetCurrentProcessId());
   freopen_s(&conin, "CONIN$", "r", stdin);
   freopen_s(&conout, "CONOUT$", "w", stdout);
   freopen_s(&conerr, "CONOUT$", "w", stderr);
   SetConsoleTitleW(L"Triglav Debug Console");

   enable_virtual_terminal_processing();
}

}// namespace

int WINAPI WinMain(HINSTANCE /*h_instance*/, HINSTANCE /*h_prev_instance*/, PSTR /*lp_cmd_line*/, int /*n_cmd_show*/)
{
   InputArgs input_args{construct_input_args()};

#if !NDEBUG
   create_debug_console();
#endif

   auto display = triglav::desktop::get_display();
#if NDEBUG
   try {
#endif// NDEBUG
      return triglav_main(input_args, *display);
#if NDEBUG
   } catch (const std::exception& e) {
      triglav::log_message(triglav::LogLevel::Error, triglav::StringView{"DesktopMain-Windows"},
                           "desktop-main: exception occurred: {}, exiting...", e.what());
      triglav::flush_logs();
      return EXIT_FAILURE;
   } catch (...) {
      triglav::log_message(triglav::LogLevel::Error, triglav::StringView{"DesktopMain-Windows"},
                           "desktop-main: exception occurred, exiting...");
      triglav::flush_logs();
      return EXIT_FAILURE;
   }
#endif// NDEBUG
}
