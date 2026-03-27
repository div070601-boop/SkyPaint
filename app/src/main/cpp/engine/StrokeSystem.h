#pragma once

#include "../math/MathTypes.h"
#include <string>

namespace sky {

/// Manages stroke lifecycle — collecting points, finalizing strokes
class StrokeSystem {
public:
    StrokeSystem() = default;

    /// Set the active stroke color (applied to all new points)
    void setActiveColor(const Vec4& color) { m_activeColor = color; }
    const Vec4& getActiveColor() const { return m_activeColor; }

    /// Drafting mode toggles
    void setStraightLineMode(bool enable) { m_straightLineMode = enable; }
    void setGridSnap(bool enable, float size = 0.5f) { m_gridSnap = enable; m_gridSize = size; }
    void setAngleSnap(bool enable, float degrees = 15.0f) { m_angleSnap = enable; m_snapAngle = degrees; }
    
    // Get live measurement
    float getCurrentStrokeLength() const;

    /// Begin a new stroke
    void beginStroke();

    /// Add a point to the current stroke
    void addPoint(const StrokePoint& point);

    /// End the current stroke, returning its index
    int endStroke();

    /// Get all points for a given stroke
    const std::vector<StrokePoint>& getStrokePoints(int strokeIndex) const;

    /// Get the current (in-progress) stroke points
    const std::vector<StrokePoint>& getCurrentStrokePoints() const;

    /// Get total number of finalized strokes
    int getStrokeCount() const;

    /// Remove a stroke by index
    void removeStroke(int index);

    /// Push a finalized stroke back (used for Redo)
    void pushStroke(const std::vector<StrokePoint>& stroke) { 
        m_strokes.push_back(stroke); 
        m_visible.push_back(true);
        m_locked.push_back(false);
        m_names.push_back("Stroke");
    }
    
    /// Pop the last stroke (used for Undo)
    void popStroke() {
        if (!m_strokes.empty()) {
            m_strokes.pop_back();
            m_visible.pop_back();
            m_locked.pop_back();
            m_names.pop_back();
        }
    }

    /// Clear all strokes
    void clearAll();

    /// Check if currently drawing
    bool isDrawing() const { return m_isDrawing; }

    // Properties
    bool isVisible(int index) const;
    void setVisible(int index, bool visible);
    bool isLocked(int index) const;
    void setLocked(int index, bool locked);
    std::string getName(int index) const;
    void setName(int index, const std::string& name);

private:
    std::vector<std::vector<StrokePoint>> m_strokes;
    std::vector<bool> m_visible;
    std::vector<bool> m_locked;
    std::vector<std::string> m_names;

    std::vector<StrokePoint> m_currentStroke;
    bool m_isDrawing = false;
    Vec4 m_activeColor{1.0f, 1.0f, 1.0f, 1.0f};

    // Drafting state
    bool m_straightLineMode = false;
    bool m_gridSnap = false;
    bool m_angleSnap = false;
    float m_gridSize = 0.5f;
    float m_snapAngle = 15.0f;

    /// Smooth incoming points to reduce jitter
    StrokePoint smoothPoint(const StrokePoint& raw) const;
};

} // namespace sky
