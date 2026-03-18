#include "StrokeSystem.h"
#include "SnapEngine.h"

namespace feather {

void StrokeSystem::beginStroke() {
    m_currentStroke.clear();
    m_isDrawing = true;
}

float StrokeSystem::getCurrentStrokeLength() const {
    if (m_currentStroke.size() < 2) return 0.0f;
    if (m_straightLineMode) {
        return SnapEngine::getLineLength(m_currentStroke.front().position, m_currentStroke.back().position);
    }
    float length = 0.0f;
    for (size_t i = 1; i < m_currentStroke.size(); ++i) {
        length += glm::distance(m_currentStroke[i-1].position, m_currentStroke[i].position);
    }
    return length;
}

void StrokeSystem::addPoint(const StrokePoint& point) {
    if (!m_isDrawing) return;

    StrokePoint smoothed = smoothPoint(point);
    smoothed.color = m_activeColor;

    if (m_currentStroke.empty()) {
        if (m_gridSnap) {
            smoothed.position = SnapEngine::snapToGrid(smoothed.position, m_gridSize);
        }
        m_currentStroke.push_back(smoothed);
        return;
    }

    if (m_straightLineMode) {
        Vec3 targetPos = smoothed.position;
        if (m_gridSnap) {
            targetPos = SnapEngine::snapToGrid(targetPos, m_gridSize);
        }
        if (m_angleSnap) {
            targetPos = SnapEngine::snapToAngle(m_currentStroke.front().position, targetPos, m_snapAngle);
        }

        if (m_currentStroke.size() == 1) {
            smoothed.position = targetPos;
            m_currentStroke.push_back(smoothed);
        } else {
            StrokePoint& last = m_currentStroke.back();
            last.position = targetPos;
            last.pressure = smoothed.pressure;
            last.tilt = smoothed.tilt;
            last.timestamp = smoothed.timestamp;
            last.color = smoothed.color;
        }
    } else {
        float dist = glm::distance(m_currentStroke.back().position, smoothed.position);
        if (dist < 0.001f) return;

        Vec3 targetPos = smoothed.position;
        if (m_gridSnap) {
            targetPos = SnapEngine::snapToGrid(targetPos, m_gridSize);
        }
        smoothed.position = targetPos;
        m_currentStroke.push_back(smoothed);
    }
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
