#include "Logging.hpp"

#include <iostream>

namespace triglav::io {

namespace {

StringView log_level_color_escape(const LogLevel level)
{
   using namespace string_literals;
   switch (level) {
   case LogLevel::Error:
      return "\033[31m"_strv;
   case LogLevel::Warning:
      return "\033[33m"_strv;
   case LogLevel::Info:
      return "\033[32m"_strv;
   case LogLevel::Debug:
      return "\033[34m"_strv;
   }
   return ""_strv;
}

}// namespace

StreamLogger::StreamLogger(IConsoleWriter& writer) :
    m_iterator(writer),
    m_should_output_color(writer.supports_color_output())
{
}

void StreamLogger::on_log(const LogLevel level, const std::chrono::time_point<std::chrono::system_clock> tp, const StringView category,
                          const StringView log)
{
   if (m_should_output_color) {
      std::format_to(m_iterator, "[{:%Y-%m-%d %H:%M:%OS}] [{}{}\033[0m] \033[33m{}\033[0m: {}\n", tp,
                     log_level_color_escape(level).to_std(), log_level_to_string(level).to_std(), category.to_std(), log.to_std());
      return;
   }

   std::format_to(m_iterator, "[{:%Y-%m-%d %H:%M:%OS}] [{}] {}: {}\n", tp, log_level_to_string(level).to_std(), category.to_std(),
                  log.to_std());
}

}// namespace triglav::io
