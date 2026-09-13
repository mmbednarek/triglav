#pragma once

#include "triglav/Event.hpp"
#include "triglav/String.hpp"
#include "triglav/world/World.hpp"

namespace triglav::editor {

class UIWorldSystem : public world::ISystem
{
 public:
   TG_TAG_CLASS(UIWorldSystem)

   TG_EVENT(OnAddedEntity, world::EntityID /*parent*/, world::EntityID /*child*/, StringView /*label*/)
   TG_EVENT(OnRemovedEntity, world::EntityID /*entity_id*/)
   TG_EVENT(OnModifiedLabel, world::EntityID /*entity_id*/, StringView /*label*/)

   UIWorldSystem(world::Level& level);

   Name system_name() override;
   void on_level_loaded(world::Level& level) override;
   void on_removed_entities(world::Level& level, std::span<const world::EntityID> ids) override;
   void on_added_component(world::Level& level, Name component_name, world::ComponentID component_id,
                           std::span<const world::EntityID> entities) override;
   void on_modified_component(world::Level& level, Name component_name, world::ComponentID component_id,
                              std::span<const world::EntityID> entities) override;

   void discover_entities(void* user, void (*callback)(void*, world::EntityID, world::EntityID, StringView)) const;

 private:
   world::Level& m_level;

   StringView get_label(world::EntityID id, StringView default_label = "[MISSING LABEL]") const;
};

}// namespace triglav::editor
