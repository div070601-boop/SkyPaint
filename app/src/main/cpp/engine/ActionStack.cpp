#include "ActionStack.h"

namespace feather {

void ActionStack::pushAction(std::unique_ptr<Action> action) {
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
