#pragma once

#include "../math/MathTypes.h"

namespace feather {

/// Manages stroke lifecycle — collecting points, finalizing strokes
class StrokeSystem {
public:
    StrokeSystem() = default;

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

    /// Clear all strokes
    void clearAll();

    /// Check if currently drawing
    bool isDrawing() const { return m_isDrawing; }

private:
    std::vector<std::vector<StrokePoint>> m_strokes;
    std::vector<StrokePoint> m_currentStroke;
    bool m_isDrawing = false;

    /// Smooth incoming points to reduce jitter
    StrokePoint smoothPoint(const StrokePoint& raw) const;
};

} // namespace feather
