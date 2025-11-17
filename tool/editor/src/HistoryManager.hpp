#pragma once

#include "triglav/Int.hpp"
#include "triglav/Logging.hpp"

#include <array>
#include <memory>

namespace triglav::editor {

constexpr i32 HISTORY_SIZE = 32;

class IHistoryAction
{
 public:
   virtual ~IHistoryAction() = default;

   virtual void redo() = 0;
   virtual void undo() = 0;
};

class HistoryManager
{
   TG_DEFINE_LOG_CATEGORY(HistoryManager)
 public:
   void push_action(std::unique_ptr<IHistoryAction> action);
   void redo();
   void undo();

   template<typename TAction, typename... TArgs>
   void emplace_action(TArgs&&... args)
   {
      this->push_action(std::make_unique<TAction>(std::forward<TArgs>(args)...));
   }

 private:
   std::array<std::unique_ptr<IHistoryAction>, HISTORY_SIZE> m_actions;
   i32 m_history_bottom = 0;
   i32 m_history_position = 0;
   i32 m_history_top = 0;
};

}// namespace triglav::editor