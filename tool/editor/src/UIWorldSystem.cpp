#include "UIWorldSystem.hpp"

#include "triglav/engine/Engine.hpp"
#include "triglav/renderer/BindlessScene.hpp"

#include <queue>

namespace triglav::editor {

using namespace name_literals;

engine::SystemRegisterer UI_WORLD_SYSTEM_REGISTERER{{
   .constructor = [](world::Level& level) -> std::unique_ptr<world::ISystem> { return std::make_unique<UIWorldSystem>(level); },
   .components =
      std::vector<Name>{
         "triglav::world::EntityLabel"_name,
      },
}};

UIWorldSystem::UIWorldSystem(world::Level& level) :
    m_level(level)
{
}

Name UIWorldSystem::system_name()
{
   return TAG;
}

void UIWorldSystem::on_level_loaded(world::Level& /*level*/)
{
   this->discover_entities(
      this, +[](void* user, const world::EntityID parent, const world::EntityID child, const StringView label) {
         static_cast<UIWorldSystem*>(user)->event_OnAddedEntity.publish(parent, child, label);
      });
}

void UIWorldSystem::on_removed_entities(world::Level& /*level*/, std::span<const world::EntityID> ids)
{
   for (const auto id : ids) {
      event_OnRemovedEntity.publish(id);
   }
}

void UIWorldSystem::on_added_component(world::Level& /*level*/, Name /*component_name*/, world::ComponentID /*component_id*/,
                                       std::span<const world::EntityID> /*entities*/)
{
   // Nothing to do
}

void UIWorldSystem::on_modified_component(world::Level& level, Name component_name, world::ComponentID /*component_id*/,
                                          std::span<const world::EntityID> entities)
{
   if (component_name != "triglav::world::EntityLabel"_name)
      return;

   for (const auto entity_id : entities) {
      StringView label = "[MISSING LABEL]";
      const auto* label_component = level.component_opt<world::EntityLabel>(entity_id);
      if (label_component != nullptr) {
         label = label_component->label;
      }

      event_OnModifiedLabel.publish(entity_id, label);
   }
}

void UIWorldSystem::discover_entities(void* user, void (*callback)(void*, world::EntityID, world::EntityID, StringView label)) const
{
   // Visit everything
   std::queue<world::EntityID> queue;
   queue.emplace(world::ROOT_ENTITY);

   callback(user, world::NO_ENTITY, world::ROOT_ENTITY, this->get_label(world::ROOT_ENTITY, "Level Root"));

   while (!queue.empty()) {
      const auto entity_id = queue.front();
      queue.pop();

      for (const auto child : m_level.children_of(entity_id)) {
         queue.emplace(child);
         callback(user, entity_id, child, this->get_label(child));
      }
   }
}

StringView UIWorldSystem::get_label(const world::EntityID id, const StringView default_label) const
{
   auto* label_component = m_level.component_opt<world::EntityLabel>(id);
   if (label_component == nullptr) {
      return default_label;
   }
   return label_component->label;
}

}// namespace triglav::editor
