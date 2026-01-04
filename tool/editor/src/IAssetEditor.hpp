#pragma once

#include "Command.hpp"

#include "triglav/Name.hpp"

namespace triglav::editor {

class IAssetEditor
{
 public:
   virtual ~IAssetEditor() = default;
   virtual void on_command(Command command) = 0;
   virtual void tick(float delta_time) = 0;
   [[nodiscard]] virtual ResourceName asset_name() const = 0;
   [[nodiscard]] virtual bool accepts_key_chords() const = 0;
};

}// namespace triglav::editor