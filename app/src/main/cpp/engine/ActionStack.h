#pragma once
#include <memory>
#include <vector>

namespace feather {

/// Base interface for any undoable operation
class Action {
public:
    virtual ~Action() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
};

/// Manages the history of actions for undo/redo
class ActionStack {
public:
    void pushAction(std::unique_ptr<Action> action);
    void undo();
    void redo();
    void clear();

    bool canUndo() const { return m_currentIndex >= 0; }
    bool canRedo() const { return m_currentIndex + 1 < static_cast<int>(m_actions.size()); }

private:
    std::vector<std::unique_ptr<Action>> m_actions;
    int m_currentIndex = -1; // Index of the last executed action
};

} // namespace feather
