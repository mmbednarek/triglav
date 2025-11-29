#pragma once

#include "Iterator.hpp"
#include "Stream.hpp"
#include "triglav/Logging.hpp"

namespace triglav::io {

class StreamLogger final : public ILogListener
{
 public:
   explicit StreamLogger(IConsoleWriter& writer);

   void on_log(LogLevel level, std::chrono::time_point<std::chrono::system_clock> tp, StringView category, StringView log) override;

 private:
   WriterIterator<char> m_iterator;
   bool m_should_output_color{true};
};

}// namespace triglav::io