#include "HistoryManager.hpp"

namespace triglav::editor {

void HistoryManager::push_action(std::unique_ptr<IHistoryAction> action)
{
   m_actions[m_history_position] = std::move(action);
   m_history_position = (m_history_position + 1) % HISTORY_SIZE;
   if (m_history_position == m_history_bottom) {
      m_history_bottom = (m_history_bottom + 1) % HISTORY_SIZE;
   }
   m_history_top = m_history_position;

   log_debug("Bottom: {}, Mid: {}, Top: {}", m_history_bottom, m_history_position, m_history_top);
}

void HistoryManager::redo()
{
   if (m_history_position != m_history_top) {
      m_actions[m_history_position]->redo();
      m_history_position = (m_history_position + 1) % HISTORY_SIZE;
   }

   log_debug("Bottom: {}, Mid: {}, Top: {}", m_history_bottom, m_history_position, m_history_top);
}

void HistoryManager::undo()
{
   if (m_history_position != m_history_bottom) {
      m_history_position = (m_history_position - 1) % HISTORY_SIZE;
      m_actions[m_history_position]->undo();
   }

   log_debug("Bottom: {}, Mid: {}, Top: {}", m_history_bottom, m_history_position, m_history_top);
}

}// namespace triglav::editor
