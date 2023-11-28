#include "Parser.h"

#include <iostream>

namespace {
bool is_white_char(const char ch)
{
   return ch == ' ' || ch == '\t' || ch == '\r';
}
}// namespace

namespace object_reader {

Parser::Parser(std::istream &stream) :
    m_reader(stream)
{
}

void Parser::parse()
{
   while (m_reader.has_next()) {
      const auto ch = m_reader.next();
      if (ch == 0)
         continue;

      if (m_state == State::LineStart) {
         if (ch == '#') {
            m_state = State::Comment;
            continue;
         }
         if (ch == '\n') {
            continue;
         }
         if (is_white_char(ch)) {
            continue;
         }

         m_state = State::Command;
      }
      if (m_state == State::Comment) {
         if (ch == '\n') {
            m_state = State::LineStart;
         }
         continue;
      }

      if (is_white_char(ch)) {
         this->process_token();
         continue;
      }
      if (ch == '\n') {
         this->process_line_end();
         continue;
      }

      m_currentToken.push_back(ch);
   }

   this->process_token();
}

void Parser::process_token()
{
   if (m_currentToken.empty())
      return;

   if (m_state == State::Command) {
      m_currentCommand.name = m_currentToken;
      m_currentToken.clear();
      m_state = State::Argument;
      return;
   }
   if (m_state == State::Argument) {
      m_currentCommand.arguments.push_back(m_currentToken);
      m_currentToken.clear();
   }
}

void Parser::process_line_end()
{
   this->process_token();

   m_state = State::LineStart;

   if (not m_currentCommand.name.empty()) {
      m_commands.push_back(m_currentCommand);
   }

   m_currentCommand.arguments.clear();
   m_currentCommand.name.clear();
}

const std::vector<Command>& Parser::commands() const
{
   return m_commands;
}

}// namespace object_reader