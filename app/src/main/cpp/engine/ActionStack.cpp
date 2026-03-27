#include "ActionStack.h"

namespace feather {

void ActionStack::pushAction(std::unique_ptr<Action> action) {
    // Check if we can merge with the previous action (for rapid continuous actions like drag)
    if (m_currentIndex >= 0 && m_actions[m_currentIndex]->canMergeWith(action.get())) {
        int64_t diffNs = action->getTimestamp() - m_actions[m_currentIndex]->getTimestamp();
        // 500 ms threshold for merging
        if (diffNs < 500000000LL) {
            m_actions[m_currentIndex]->merge(action.get());
            // Update the timestamp of the merged action to keep the chain alive
            m_actions[m_currentIndex]->setTimestamp(action->getTimestamp());
            return; // Merged successfully, do not push
        }
    }

    // If we are not at the end of the stack, erase all subsequent actions
    if (m_currentIndex + 1 < static_cast<int>(m_actions.size())) {
        m_actions.erase(m_actions.begin() + m_currentIndex + 1, m_actions.end());
    }
    m_actions.push_back(std::move(action));
    m_currentIndex++;
    
    // Limit stack size to prevent unbounded memory growth
    if (m_actions.size() > 50) {
        m_actions.erase(m_actions.begin());
        m_currentIndex--;
    }
}

void ActionStack::undo() {
    if (canUndo()) {
        m_actions[m_currentIndex]->undo();
        m_currentIndex--;
    }
}

void ActionStack::redo() {
    if (canRedo()) {
        m_currentIndex++;
        m_actions[m_currentIndex]->redo();
    }
}

void ActionStack::clear() {
    m_actions.clear();
    m_currentIndex = -1;
}

} // namespace feather
