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
}

void HistoryManager::redo()
{
   if (m_history_position != m_history_top) {
      m_actions[m_history_position]->redo();
      m_history_position = (m_history_position + 1) % HISTORY_SIZE;
   }
}

void HistoryManager::undo()
{
   if (m_history_position != m_history_bottom) {
      m_history_position = (m_history_position - 1) % HISTORY_SIZE;
      m_actions[m_history_position]->undo();
   }
}

}// namespace triglav::editor
