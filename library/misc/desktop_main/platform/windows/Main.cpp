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
   LPWSTR* cmdArgsW = CommandLineToArgvW(GetCommandLineW(), &count);
   if (cmdArgsW == nullptr) {
      return {nullptr, 0};
   }

   int argBufferSize = 0;
   for (int i = 0; i < count; ++i) {
      const auto byteCount = WideCharToMultiByte(CP_UTF8, 0, cmdArgsW[i], -1, nullptr, 0, nullptr, nullptr);
      if (byteCount == 0) {
         LocalFree(cmdArgsW);
         return {nullptr, 0};
      }

      argBufferSize += byteCount + 1;
   }

   auto* args = new char*[count + 1];
   auto* argBuffer = new char[argBufferSize];

   char* arg = argBuffer;
   for (int i = 0; i < count; ++i) {
      args[i] = arg;
      const auto bytesWritten = WideCharToMultiByte(CP_UTF8, 0, cmdArgsW[i], -1, arg, argBufferSize, nullptr, nullptr);
      if (bytesWritten == 0) {
         delete[] args;
         delete[] argBuffer;
         LocalFree(cmdArgsW);
         return {nullptr, 0};
      }

      const auto argLen = bytesWritten + 1;
      arg += argLen;
      argBufferSize -= argLen;
   }

   args[count] = nullptr;

   LocalFree(cmdArgsW);
   return {const_cast<const char**>(args), count};
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
}

}// namespace

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
   InputArgs inputArgs{construct_input_args()};

#if !NDEBUG
   create_debug_console();
#endif

   auto display = triglav::desktop::get_display();
#if NDEBUG
   try {
#endif// NDEBUG
      return triglav_main(inputArgs, *display);
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
