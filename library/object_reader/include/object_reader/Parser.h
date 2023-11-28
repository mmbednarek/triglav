#pragma once

#include <string>
#include <vector>

#include "BufferedReader.h"

namespace object_reader {

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
   explicit Parser(std::istream& stream);

   void parse();
   void process_token();
   void process_line_end();

   [[nodiscard]] const std::vector<Command>& commands() const;

private:
   BufferedReader m_reader;
   std::vector<Command> m_commands;
   State m_state{State::LineStart};
   std::string m_currentToken{};
   Command m_currentCommand;
};

}