#include "StrokeSystem.h"

namespace feather {

void StrokeSystem::beginStroke() {
    m_currentStroke.clear();
    m_isDrawing = true;
}

void StrokeSystem::addPoint(const StrokePoint& point) {
    if (!m_isDrawing) return;

    StrokePoint smoothed = smoothPoint(point);
    smoothed.color = m_activeColor;  // Apply current color

    // Minimum distance filter to avoid over-sampling
    if (!m_currentStroke.empty()) {
        float dist = glm::distance(m_currentStroke.back().position, smoothed.position);
        if (dist < 0.001f) return; // skip too-close points
    }

    m_currentStroke.push_back(smoothed);
}

int StrokeSystem::endStroke() {
    if (!m_isDrawing) return -1;
    m_isDrawing = false;

    if (m_currentStroke.size() < 2) {
        m_currentStroke.clear();
        return -1; // too few points, discard
    }

    m_strokes.push_back(std::move(m_currentStroke));
    m_currentStroke.clear();
    return static_cast<int>(m_strokes.size()) - 1;
}

const std::vector<StrokePoint>& StrokeSystem::getStrokePoints(int strokeIndex) const {
    static const std::vector<StrokePoint> empty;
    if (strokeIndex < 0 || strokeIndex >= static_cast<int>(m_strokes.size())) {
        return empty;
    }
    return m_strokes[strokeIndex];
}

const std::vector<StrokePoint>& StrokeSystem::getCurrentStrokePoints() const {
    return m_currentStroke;
}

int StrokeSystem::getStrokeCount() const {
    return static_cast<int>(m_strokes.size());
}

void StrokeSystem::removeStroke(int index) {
    if (index >= 0 && index < static_cast<int>(m_strokes.size())) {
        m_strokes.erase(m_strokes.begin() + index);
    }
}

void StrokeSystem::clearAll() {
    m_strokes.clear();
    m_currentStroke.clear();
    m_isDrawing = false;
}

StrokePoint StrokeSystem::smoothPoint(const StrokePoint& raw) const {
    if (m_currentStroke.empty()) return raw;

    // Simple exponential smoothing
    const float alpha = 0.3f;
    const StrokePoint& prev = m_currentStroke.back();

    StrokePoint smoothed;
    smoothed.position = glm::mix(prev.position, raw.position, alpha);
    smoothed.pressure = glm::mix(prev.pressure, raw.pressure, alpha);
    smoothed.tilt = glm::mix(prev.tilt, raw.tilt, alpha);
    smoothed.timestamp = raw.timestamp;

    return smoothed;
}

} // namespace feather
