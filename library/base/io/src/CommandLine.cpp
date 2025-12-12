#include "CommandLine.hpp"

namespace triglav::io {

CommandLine& CommandLine::the()
{
   static CommandLine instance;
   return instance;
}

void CommandLine::parse(const int argc, const char** argv)
{
   std::string pending_argument;
   for (int i = 1; i < argc; ++i) {
      std::string arg{argv[i]};
      if (*arg.begin() == '-') {
         if (not pending_argument.empty()) {
            m_enabled_flags.emplace(make_name_id(pending_argument));
         }

         auto equal_sign_at = arg.find('=');
         if (equal_sign_at != std::string::npos) {
            m_arguments.emplace(make_name_id(arg.substr(1, equal_sign_at - 1)), arg.substr(equal_sign_at + 1));
            pending_argument.clear();
         } else {
            pending_argument = arg.substr(1);
         }
      } else {
         if (not pending_argument.empty()) {
            m_arguments.emplace(make_name_id(pending_argument), arg);
            pending_argument.clear();
         }
      }
   }

   if (not pending_argument.empty()) {
      m_enabled_flags.emplace(make_name_id(pending_argument));
   }
}

bool CommandLine::is_enabled(Name flag) const
{
   return m_enabled_flags.contains(flag);
}

std::optional<std::string> CommandLine::arg(Name flag) const
{
   const auto it = m_arguments.find(flag);
   if (it == m_arguments.end()) {
      return std::nullopt;
   }
   return it->second;
}

std::optional<int> CommandLine::arg_int(Name flag) const
{
   auto arg_str = this->arg(flag);
   if (not arg_str.has_value()) {
      return std::nullopt;
   }
   return std::stoi(*arg_str);
}

}// namespace triglav::io