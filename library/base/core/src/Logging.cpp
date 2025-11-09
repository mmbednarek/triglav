#include "Logging.hpp"

#include <cstring>
#include <iostream>

// Global TG_LOG_LEVEL defined as log level function
// needs to be undefined here
#undef TG_LOG_LEVEL

namespace triglav {

StringView log_level_to_string(const LogLevel level)
{
   using namespace string_literals;
   switch (level) {
#define TG_LOG_LEVEL(long, short, arg) \
   case LogLevel::long:                \
      return #long##_strv;
      TG_LOG_LEVEL_LIST(0)
#undef TG_LOG_LEVEL
   default:
      return ""_strv;
   }
}

LogManager::LogManager() :
    m_buffer(LOG_BUFFER_SIZE)
{
}

LogManager& LogManager::the()
{
   static LogManager instance;
   return instance;
}

void LogManager::write_log(const LogHeader& header, const char* category_buffer, const char* log_buffer)
{
   std::unique_lock lk{m_access_mutex};

   const auto total_size = sizeof(LogHeader) + sizeof(char) * header.category_size + sizeof(char) * header.log_size;
   assert(total_size <= LOG_BUFFER_SIZE);
   if (m_offset + total_size > LOG_BUFFER_SIZE) {
      this->flush_internal();
   }

   std::memcpy(m_buffer.data() + m_offset, &header, sizeof(LogHeader));
   std::memcpy(m_buffer.data() + m_offset + sizeof(LogHeader), category_buffer, sizeof(char) * header.category_size);
   std::memcpy(m_buffer.data() + m_offset + sizeof(LogHeader) + sizeof(char) * header.category_size, log_buffer,
               sizeof(char) * header.log_size);
   m_offset += total_size;
}

void LogManager::flush()
{
   std::unique_lock lk{m_access_mutex};
   this->flush_internal();
}

void LogManager::flush_internal()
{
   MemorySize read_offset = 0;

   while (read_offset < m_offset) {
      const auto* header = reinterpret_cast<const LogHeader*>(m_buffer.data() + read_offset);
      const auto* category_ptr = reinterpret_cast<const char*>(m_buffer.data() + read_offset + sizeof(LogHeader));
      const auto* log_ptr =
         reinterpret_cast<const char*>(m_buffer.data() + read_offset + sizeof(LogHeader) + sizeof(char) * header->category_size);

      for (const auto& listener : m_listeners) {
         listener->on_log(header->level, header->timestamp, {category_ptr, header->category_size}, {log_ptr, header->log_size});
      }

      read_offset += sizeof(LogHeader) + sizeof(char) * header->category_size + sizeof(char) * header->log_size;
   }

   m_offset = 0;
}

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

void flush_logs()
{
   LogManager::the().flush();
}

void StdOutLogger::on_log(const LogLevel level, const std::chrono::time_point<std::chrono::system_clock> tp, const StringView category,
                          const StringView log)
{
   std::cout << std::format("[{:%Y-%m-%d %H:%M:%OS}] [{}{}\033[0m] \033[33m{}\033[0m: {}\n", tp, log_level_color_escape(level).to_std(),
                            log_level_to_string(level).to_std(), category.to_std(), log.to_std());
}

}// namespace triglav