#pragma once
#include <memory>
#include <vector>
#include <chrono>
#include <cstdint>

namespace sky {

/// Base interface for any undoable operation
class Action {
public:
    Action() {
        m_timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    }
    virtual ~Action() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;

    virtual int getTypeId() const { return 0; }
    virtual bool canMergeWith(const Action*) const { return false; }
    virtual void merge(const Action*) {}

    int64_t getTimestamp() const { return m_timestamp; }
    void setTimestamp(int64_t ts) { m_timestamp = ts; }

protected:
    int64_t m_timestamp;
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

} // namespace sky
