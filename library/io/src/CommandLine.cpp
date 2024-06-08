#include "CommandLine.h"

namespace triglav::io {

CommandLine& CommandLine::the()
{
   static CommandLine instance;
   return instance;
}

void CommandLine::parse(const int argc, const char** argv)
{
   std::string pendingArgument;
   for (int i = 1; i < argc; ++i) {
      std::string arg{argv[i]};
      if (*arg.begin() == '-') {
         if (not pendingArgument.empty()) {
            m_enabledFlags.emplace(make_name_id(pendingArgument));
         }

         auto equalSignAt = arg.find('=');
         if (equalSignAt != std::string::npos) {
            m_arguments.emplace(make_name_id(arg.substr(1, equalSignAt - 1)), arg.substr(equalSignAt + 1));
            pendingArgument.clear();
         } else {
            pendingArgument = arg.substr(1);
         }
      } else {
         if (not pendingArgument.empty()) {
            m_arguments.emplace(make_name_id(pendingArgument), arg);
            pendingArgument.clear();
         }
      }
   }

   if (not pendingArgument.empty()) {
      m_enabledFlags.emplace(make_name_id(pendingArgument));
   }
}

bool CommandLine::is_enabled(Name flag) const
{
   return m_enabledFlags.contains(flag);
}

std::optional<std::string> CommandLine::arg(Name flag) const
{
   const auto it = m_arguments.find(flag);
   if (it == m_arguments.end()) {
      return std::nullopt;
   }
   return it->second;
}

}// namespace triglav::io