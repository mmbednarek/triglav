#include "HistoryManager.hpp"

namespace triglav::editor {

void HistoryManager::push_action(std::unique_ptr<IHistoryAction> action)
{
   m_actions[m_historyPosition] = std::move(action);
   m_historyPosition = (m_historyPosition + 1) % HISTORY_SIZE;
   if (m_historyPosition == m_historyBottom) {
      m_historyBottom = (m_historyBottom + 1) % HISTORY_SIZE;
   }
   m_historyTop = m_historyPosition;

   log_debug("Bottom: {}, Mid: {}, Top: {}", m_historyBottom, m_historyPosition, m_historyTop);
}

void HistoryManager::redo()
{
   if (m_historyPosition != m_historyTop) {
      m_actions[m_historyPosition]->redo();
      m_historyPosition = (m_historyPosition + 1) % HISTORY_SIZE;
   }

   log_debug("Bottom: {}, Mid: {}, Top: {}", m_historyBottom, m_historyPosition, m_historyTop);
}

void HistoryManager::undo()
{
   if (m_historyPosition != m_historyBottom) {
      m_historyPosition = (m_historyPosition - 1) % HISTORY_SIZE;
      m_actions[m_historyPosition]->undo();
   }

   log_debug("Bottom: {}, Mid: {}, Top: {}", m_historyBottom, m_historyPosition, m_historyTop);
}

}// namespace triglav::editor
