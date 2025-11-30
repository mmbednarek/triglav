#include "CommandManager.hpp"

namespace triglav::editor {

void CommandManager::register_command(const Name menu_item, const KeyChord chord, const Command cmd)
{
   m_menu_item_to_command[menu_item] = cmd;
   m_chords_to_command[chord] = cmd;
}

void CommandManager::register_command(const KeyChord chord, const Command cmd)
{
   m_chords_to_command[chord] = cmd;
}

void CommandManager::register_command(const Name menu_item, const Command cmd)
{
   m_menu_item_to_command[menu_item] = cmd;
}

std::optional<Command> CommandManager::translate_chord(const KeyChord chord) const
{
   const auto it = m_chords_to_command.find(chord);
   if (it == m_chords_to_command.end())
      return std::nullopt;
   return it->second;
}

std::optional<Command> CommandManager::translate_menu_item(const Name menu_item) const
{
   const auto it = m_menu_item_to_command.find(menu_item);
   if (it == m_menu_item_to_command.end())
      return std::nullopt;
   return it->second;
}

}// namespace triglav::editor
