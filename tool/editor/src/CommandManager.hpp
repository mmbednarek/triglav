#pragma once

#include "Command.hpp"

#include "triglav/Name.hpp"
#include "triglav/desktop/Desktop.hpp"

#include <map>
#include <optional>

namespace triglav::editor {

struct KeyChord
{
   desktop::ModifierFlags modifiers{};
   desktop::Key key{};

   [[nodiscard]] auto operator<=>(const KeyChord& other) const
   {
      if (modifiers.value != other.modifiers.value) {
         return modifiers.value <=> other.modifiers.value;
      }
      return key <=> other.key;
   }
};

class CommandManager
{
 public:
   void register_command(Name menu_item, KeyChord chord, Command cmd);
   void register_command(KeyChord chord, Command cmd);
   void register_command(Name menu_item, Command cmd);
   [[nodiscard]] std::optional<Command> translate_chord(KeyChord chord) const;
   [[nodiscard]] std::optional<Command> translate_menu_item(Name menu_item) const;

 private:
   std::map<KeyChord, Command> m_chords_to_command;
   std::map<Name, Command> m_menu_item_to_command;
};

}// namespace triglav::editor