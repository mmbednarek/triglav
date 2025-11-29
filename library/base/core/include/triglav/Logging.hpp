#pragma once

#include "Int.hpp"
#include "String.hpp"

#include <chrono>
#include <format>
#include <memory>
#include <mutex>
#include <vector>

#define TG_LOG_LEVEL_LIST(arg)      \
   TG_LOG_LEVEL(Error, error, arg)  \
   TG_LOG_LEVEL(Warning, warn, arg) \
   TG_LOG_LEVEL(Info, info, arg)    \
   TG_LOG_LEVEL(Debug, debug, arg)

namespace triglav {

enum class LogLevel
{
#define TG_LOG_LEVEL(long, short, arg) long,
   TG_LOG_LEVEL_LIST(0)
#undef TG_LOG_LEVEL
};

[[nodiscard]] StringView log_level_to_string(LogLevel level);

struct LogHeader
{
   LogLevel level;
   std::chrono::time_point<std::chrono::system_clock> timestamp;
   u32 category_size{};
   u32 log_size{};
};

class ILogListener
{
 public:
   virtual ~ILogListener() = default;

   virtual void on_log(LogLevel level, std::chrono::time_point<std::chrono::system_clock> tp, StringView category, StringView log) = 0;
};

class LogManager
{
 public:
   static constexpr MemorySize LOG_BUFFER_SIZE = 2048;

   LogManager();

   static LogManager& the();

   void write_log(const LogHeader& header, const char* category_buffer, const char* log_buffer);
   void flush();

   template<typename... T>
   void write_formatted(const LogHeader& header, const char* category_buffer, std::format_string<T...> fmt, T&&... args)
   {
      std::unique_lock lk{m_access_mutex};

      auto log_offset = m_offset + sizeof(LogHeader) + sizeof(char) * header.category_size;
      if (log_offset > m_buffer.size()) {
         this->flush_internal();
         log_offset = m_offset + sizeof(LogHeader) + sizeof(char) * header.category_size;
         assert(log_offset <= m_buffer.size());
      }

      std::memcpy(m_buffer.data() + m_offset + sizeof(LogHeader), category_buffer, sizeof(char) * header.category_size);

      auto log_ptr = m_buffer.data() + log_offset;
      const auto end = std::format_to_n(log_ptr, m_buffer.size() - log_offset, fmt, std::forward<T>(args)...);

      const auto total_size = sizeof(LogHeader) + sizeof(char) * header.category_size + sizeof(char) * end.size;
      if (m_offset + total_size > m_buffer.size()) {
         assert(total_size <= m_buffer.size());
         this->flush_internal();
         lk.unlock();
         write_formatted(header, category_buffer, fmt, std::forward<T>(args)...);
         return;
      }

      LogHeader new_header(header);
      new_header.log_size = static_cast<u32>(end.size);

      std::memcpy(m_buffer.data() + m_offset, &new_header, sizeof(LogHeader));

      m_offset += total_size;
   }

   template<typename T, typename... TArgs>
   void register_listener(TArgs&&... args)
   {
      std::unique_lock lk{m_access_mutex};
      m_listeners.emplace_back(std::make_unique<T>(std::forward<TArgs>(args)...));
   }

 private:
   void flush_internal();

   std::mutex m_access_mutex;
   std::vector<u8> m_buffer;
   MemorySize m_offset{};
   std::vector<std::unique_ptr<ILogListener>> m_listeners;
};

void flush_logs();

template<typename... TArgs>
void log_message(const LogLevel level, const StringView category, std::format_string<TArgs...> fmt, TArgs&&... args)
{
   LogHeader header;
   header.level = level;
   header.timestamp = std::chrono::system_clock::now();
   header.category_size = static_cast<u32>(category.size());

   LogManager::the().write_formatted(header, category.data(), fmt, std::forward<TArgs>(args)...);
}

}// namespace triglav

#define TG_LOG_LEVEL(long, short, name)                                                                   \
   template<typename... TArgs>                                                                            \
   static void log_##short(std::format_string<TArgs...> fmt, TArgs && ... args)                           \
   {                                                                                                      \
      using namespace ::triglav::string_literals;                                                         \
      ::triglav::log_message(::triglav::LogLevel::long, #name##_strv, fmt, std::forward<TArgs>(args)...); \
   }

#define TG_DEFINE_LOG_CATEGORY(name) TG_LOG_LEVEL_LIST(name)
