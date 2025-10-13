#pragma once

#include <string>
#include <vector>

#include "triglav/io/BufferedReader.hpp"

namespace triglav::geometry {

enum class State
{
   LineStart,
   Comment,
   Command,
   Argument
};

struct Command
{
   std::string name;
   std::vector<std::string> arguments;
};

class Parser
{
 public:
   explicit Parser(io::IReader& stream);

   void parse();
   void process_token();
   void process_line_end();

   [[nodiscard]] const std::vector<Command>& commands() const;

 private:
   io::BufferedReader m_reader;
   std::vector<Command> m_commands;
   State m_state{State::LineStart};
   std::string m_currentToken{};
   Command m_currentCommand;
};

}// namespace triglav::geometry