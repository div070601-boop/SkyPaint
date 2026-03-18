#pragma once

#include "ActionStack.h"
#include "StrokeSystem.h"
#include "VoxelGrid.h"
#include <vector>

namespace feather {

/// Command to undo or redo a 3D stroke
class StrokeAction : public Action {
public:
    StrokeAction(StrokeSystem& strokeSystem, const std::vector<StrokePoint>& stroke)
        : m_strokeSystem(strokeSystem), m_strokeBasePoints(stroke) {}

    void undo() override {
        m_strokeSystem.popStroke();
    }

    void redo() override {
        m_strokeSystem.pushStroke(m_strokeBasePoints);
    }

private:
    StrokeSystem& m_strokeSystem;
    std::vector<StrokePoint> m_strokeBasePoints;
};

/// Command to undo or redo a Voxel Sculpting operation using full-grid snapshots
class SculptAction : public Action {
public:
    SculptAction(VoxelGrid& voxelGrid, const std::vector<float>& gridSnapshot)
        : m_voxelGrid(voxelGrid), m_snapshot(gridSnapshot) {}

    void undo() override {
        // Save the current state so we can redo it
        m_redoSnapshot = m_voxelGrid.getField();
        m_voxelGrid.setField(m_snapshot);
    }

    void redo() override {
        m_voxelGrid.setField(m_redoSnapshot);
    }

private:
    VoxelGrid& m_voxelGrid;
    std::vector<float> m_snapshot;
    std::vector<float> m_redoSnapshot;
};

} // namespace feather
