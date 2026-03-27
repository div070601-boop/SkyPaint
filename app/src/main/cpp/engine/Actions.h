#pragma once

#include "ActionStack.h"
#include "StrokeSystem.h"
#include "VoxelGrid.h"
#include "SceneObject.h"
#include "../math/MathTypes.h"
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

/// Command to undo/redo adding a primitive
class PrimitiveAddAction : public Action {
public:
    PrimitiveAddAction(std::vector<SceneObject>& primitives, const SceneObject& obj)
        : m_primitives(primitives), m_object(obj) {}

    void undo() override {
        if (!m_primitives.empty()) {
            m_primitives.pop_back();
        }
    }

    void redo() override {
        m_primitives.push_back(m_object);
    }

private:
    std::vector<SceneObject>& m_primitives;
    SceneObject m_object;
};

/// Command to undo/redo a primitive transform change
class PrimitiveTransformAction : public Action {
public:
    PrimitiveTransformAction(std::vector<SceneObject>& primitives, int index,
                             const Mat4& oldTransform, const Mat4& newTransform)
        : m_primitives(primitives), m_index(index),
          m_oldTransform(oldTransform), m_newTransform(newTransform) {}

    void undo() override {
        if (m_index >= 0 && m_index < static_cast<int>(m_primitives.size())) {
            m_primitives[m_index].transform = m_oldTransform;
        }
    }

    void redo() override {
        if (m_index >= 0 && m_index < static_cast<int>(m_primitives.size())) {
            m_primitives[m_index].transform = m_newTransform;
        }
    }

private:
    std::vector<SceneObject>& m_primitives;
    int m_index;
    Mat4 m_oldTransform;
    Mat4 m_newTransform;
};

} // namespace feather
