#include <jni.h>
#include <android/log.h>
#include <memory>
#include <string>
#include "engine/GeometryEngine.h"
#include <glm/gtc/type_ptr.hpp>

#define LOG_TAG "Feather3D-JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::unique_ptr<feather::GeometryEngine> g_engine;

extern "C" {

// ── Engine Lifecycle ────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_nativeInit(JNIEnv* env, jclass) {
    g_engine = std::make_unique<feather::GeometryEngine>();
    LOGI("GeometryEngine initialized");
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_nativeDestroy(JNIEnv* env, jclass) {
    g_engine.reset();
    LOGI("GeometryEngine destroyed");
}

// ── Draw Mode ───────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setDrawMode(JNIEnv* env, jclass, jint mode) {
    if (!g_engine) return;
    g_engine->setDrawMode(static_cast<feather::DrawMode>(mode));
}

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_getDrawMode(JNIEnv* env, jclass) {
    if (!g_engine) return 0;
    return static_cast<jint>(g_engine->getDrawMode());
}

// ── Stroke Drawing ──────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_beginStroke(JNIEnv* env, jclass) {
    if (!g_engine) return;
    g_engine->beginStroke();
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_addStrokePoint(JNIEnv* env, jclass,
    jfloat x, jfloat y, jfloat z, jfloat pressure, jfloat tilt, jfloat timestamp) {
    if (!g_engine) return;
    g_engine->addStrokePoint(feather::Vec3(x, y, z), pressure, tilt, timestamp);
}

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_endStroke(JNIEnv* env, jclass) {
    if (!g_engine) return -1;
    return g_engine->endStroke();
}

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_getStrokeCount(JNIEnv* env, jclass) {
    if (!g_engine) return 0;
    return g_engine->getStrokeCount();
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_removeStroke(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return;
    g_engine->removeStroke(index);
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_clearStrokes(JNIEnv* env, jclass) {
    if (!g_engine) return;
    g_engine->clearStrokes();
}

JNIEXPORT jfloatArray JNICALL
Java_com_feather3d_app_NativeBridge_getStrokePoints(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return nullptr;
    const auto& points = g_engine->getStrokePoints(index);
    if (points.empty()) return nullptr;

    const int floatsPerPoint = 10;
    jfloatArray result = env->NewFloatArray(points.size() * floatsPerPoint);
    if (!result) return nullptr;

    std::vector<float> data(points.size() * floatsPerPoint);
    for (size_t i = 0; i < points.size(); ++i) {
        data[i * floatsPerPoint + 0] = points[i].position.x;
        data[i * floatsPerPoint + 1] = points[i].position.y;
        data[i * floatsPerPoint + 2] = points[i].position.z;
        data[i * floatsPerPoint + 3] = points[i].pressure;
        data[i * floatsPerPoint + 4] = points[i].tilt;
        data[i * floatsPerPoint + 5] = points[i].timestamp;
        data[i * floatsPerPoint + 6] = points[i].color.r;
        data[i * floatsPerPoint + 7] = points[i].color.g;
        data[i * floatsPerPoint + 8] = points[i].color.b;
        data[i * floatsPerPoint + 9] = points[i].color.a;
    }
    
    env->SetFloatArrayRegion(result, 0, data.size(), data.data());
    return result;
}

// ── Tube Parameters ─────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setTubeRadius(JNIEnv* env, jclass, jfloat radius) {
    if (!g_engine) return;
    g_engine->setTubeRadius(radius);
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setTubeSegments(JNIEnv* env, jclass, jint segments) {
    if (!g_engine) return;
    g_engine->setTubeSegments(segments);
}

// ── Stroke Color ────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setStrokeColor(JNIEnv* env, jclass,
    jfloat r, jfloat g, jfloat b, jfloat a) {
    if (!g_engine) return;
    g_engine->setStrokeColor(r, g, b, a);
}

// ── Drafting State ──────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setStraightLineMode(JNIEnv* env, jclass, jboolean enable) {
    if (!g_engine) return;
    g_engine->setStraightLineMode(enable == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setGridSnap(JNIEnv* env, jclass, jboolean enable, jfloat size) {
    if (!g_engine) return;
    g_engine->setGridSnap(enable == JNI_TRUE, size);
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setAngleSnap(JNIEnv* env, jclass, jboolean enable, jfloat degrees) {
    if (!g_engine) return;
    g_engine->setAngleSnap(enable == JNI_TRUE, degrees);
}

JNIEXPORT jfloat JNICALL
Java_com_feather3d_app_NativeBridge_getCurrentStrokeLength(JNIEnv* env, jclass) {
    if (!g_engine) return 0.0f;
    return g_engine->getCurrentStrokeLength();
}

// ── Voxel Sculpting ─────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_initVoxelGrid(JNIEnv* env, jclass,
    jint resolution, jfloat minX, jfloat minY, jfloat minZ,
    jfloat maxX, jfloat maxY, jfloat maxZ) {
    if (!g_engine) return;
    g_engine->initVoxelGrid(resolution,
        feather::Vec3(minX, minY, minZ),
        feather::Vec3(maxX, maxY, maxZ));
    LOGI("VoxelGrid initialized: res=%d", resolution);
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_sculptAt(JNIEnv* env, jclass,
    jfloat x, jfloat y, jfloat z, jfloat radius, jfloat strength) {
    if (!g_engine) return;
    g_engine->sculptAt(feather::Vec3(x, y, z), radius, strength);
}

// ── Liquify ─────────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setLiquifyRadius(JNIEnv* env, jclass, jfloat radius) {
    if (!g_engine) return;
    g_engine->setLiquifyRadius(radius);
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_setLiquifyStrength(JNIEnv* env, jclass, jfloat strength) {
    if (!g_engine) return;
    g_engine->setLiquifyStrength(strength);
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_liquifyAt(JNIEnv* env, jclass,
    jfloat px, jfloat py, jfloat pz, jfloat dx, jfloat dy, jfloat dz) {
    if (!g_engine) return;
    g_engine->liquifyAt(feather::Vec3(px, py, pz), feather::Vec3(dx, dy, dz));
}

// ── Decimation ──────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_decimateVoxelMesh(JNIEnv* env, jclass, jfloat ratio) {
    if (!g_engine) return;
    g_engine->decimateVoxelMesh(ratio);
}

// ── Mesh Data Retrieval (for renderer) ──────────────────────────────────────

// Returns interleaved vertex data: [pos.x, pos.y, pos.z, norm.x, norm.y, norm.z, u, v, r, g, b, a, ...]
static jfloatArray meshVerticesToFloatArray(JNIEnv* env, const feather::Mesh& mesh) {
    int floatsPerVertex = 12; // 3 pos + 3 normal + 2 uv + 4 color
    int totalFloats = static_cast<int>(mesh.vertices.size()) * floatsPerVertex;

    jfloatArray result = env->NewFloatArray(totalFloats);
    if (!result) return nullptr;

    std::vector<float> data(totalFloats);
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const auto& v = mesh.vertices[i];
        size_t base = i * floatsPerVertex;
        data[base + 0] = v.position.x;
        data[base + 1] = v.position.y;
        data[base + 2] = v.position.z;
        data[base + 3] = v.normal.x;
        data[base + 4] = v.normal.y;
        data[base + 5] = v.normal.z;
        data[base + 6] = v.uv.x;
        data[base + 7] = v.uv.y;
        data[base + 8] = v.color.r;
        data[base + 9] = v.color.g;
        data[base + 10] = v.color.b;
        data[base + 11] = v.color.a;
    }

    env->SetFloatArrayRegion(result, 0, totalFloats, data.data());
    return result;
}

static jintArray meshIndicesToIntArray(JNIEnv* env, const feather::Mesh& mesh) {
    int totalIndices = static_cast<int>(mesh.indices.size());
    jintArray result = env->NewIntArray(totalIndices);
    if (!result) return nullptr;

    std::vector<jint> data(mesh.indices.begin(), mesh.indices.end());
    env->SetIntArrayRegion(result, 0, totalIndices, data.data());
    return result;
}

JNIEXPORT jfloatArray JNICALL
Java_com_feather3d_app_NativeBridge_getStrokeMeshVertices(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return nullptr;
    const auto& mesh = g_engine->getStrokeMesh(index);
    return meshVerticesToFloatArray(env, mesh);
}

JNIEXPORT jintArray JNICALL
Java_com_feather3d_app_NativeBridge_getStrokeMeshIndices(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return nullptr;
    const auto& mesh = g_engine->getStrokeMesh(index);
    return meshIndicesToIntArray(env, mesh);
}

JNIEXPORT jfloatArray JNICALL
Java_com_feather3d_app_NativeBridge_getCombinedMeshVertices(JNIEnv* env, jclass) {
    if (!g_engine) return nullptr;
    auto mesh = g_engine->getCombinedStrokeMesh();
    return meshVerticesToFloatArray(env, mesh);
}

JNIEXPORT jintArray JNICALL
Java_com_feather3d_app_NativeBridge_getCombinedMeshIndices(JNIEnv* env, jclass) {
    if (!g_engine) return nullptr;
    auto mesh = g_engine->getCombinedStrokeMesh();
    return meshIndicesToIntArray(env, mesh);
}

JNIEXPORT jfloatArray JNICALL
Java_com_feather3d_app_NativeBridge_getCurrentStrokeMeshVertices(JNIEnv* env, jclass) {
    if (!g_engine) return nullptr;
    auto mesh = g_engine->getCurrentStrokeMesh();
    return meshVerticesToFloatArray(env, mesh);
}

JNIEXPORT jintArray JNICALL
Java_com_feather3d_app_NativeBridge_getCurrentStrokeMeshIndices(JNIEnv* env, jclass) {
    if (!g_engine) return nullptr;
    auto mesh = g_engine->getCurrentStrokeMesh();
    return meshIndicesToIntArray(env, mesh);
}

JNIEXPORT jfloatArray JNICALL
Java_com_feather3d_app_NativeBridge_getVoxelMeshVertices(JNIEnv* env, jclass) {
    if (!g_engine) return nullptr;
    auto mesh = g_engine->getVoxelMesh();
    return meshVerticesToFloatArray(env, mesh);
}

JNIEXPORT jintArray JNICALL
Java_com_feather3d_app_NativeBridge_getVoxelMeshIndices(JNIEnv* env, jclass) {
    if (!g_engine) return nullptr;
    auto mesh = g_engine->getVoxelMesh();
    return meshIndicesToIntArray(env, mesh);
}

// ── Export ───────────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_feather3d_app_NativeBridge_exportOBJ(JNIEnv* env, jclass) {
    if (!g_engine) return env->NewStringUTF("");
    std::string obj = g_engine->exportOBJ();
    return env->NewStringUTF(obj.c_str());
}

JNIEXPORT jbyteArray JNICALL
Java_com_feather3d_app_NativeBridge_exportGLB(JNIEnv* env, jclass) {
    if (!g_engine) return nullptr;
    auto data = g_engine->exportGLB();

    jbyteArray result = env->NewByteArray(static_cast<jsize>(data.size()));
    if (!result) return nullptr;
    env->SetByteArrayRegion(result, 0, static_cast<jsize>(data.size()),
                            reinterpret_cast<const jbyte*>(data.data()));
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_feather3d_app_NativeBridge_exportOBJToFile(JNIEnv* env, jclass, jstring path) {
    if (!g_engine) return JNI_FALSE;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    bool success = g_engine->exportOBJToFile(std::string(pathStr));
    env->ReleaseStringUTFChars(path, pathStr);
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_feather3d_app_NativeBridge_exportGLBToFile(JNIEnv* env, jclass, jstring path) {
    if (!g_engine) return JNI_FALSE;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    bool success = g_engine->exportGLBToFile(std::string(pathStr));
    env->ReleaseStringUTFChars(path, pathStr);
    return success ? JNI_TRUE : JNI_FALSE;
}

// ── Scene Info ──────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_getTotalVertexCount(JNIEnv* env, jclass) {
    if (!g_engine) return 0;
    return g_engine->getTotalVertexCount();
}

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_getTotalTriangleCount(JNIEnv* env, jclass) {
    if (!g_engine) return 0;
    return g_engine->getTotalTriangleCount();
}

// ── Undo / Redo ─────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_undo(JNIEnv* env, jclass) {
    if (g_engine) g_engine->undo();
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_redo(JNIEnv* env, jclass) {
    if (g_engine) g_engine->redo();
}

JNIEXPORT jboolean JNICALL
Java_com_feather3d_app_NativeBridge_canUndo(JNIEnv* env, jclass) {
    return g_engine ? (g_engine->canUndo() ? JNI_TRUE : JNI_FALSE) : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_feather3d_app_NativeBridge_canRedo(JNIEnv* env, jclass) {
    return g_engine ? (g_engine->canRedo() ? JNI_TRUE : JNI_FALSE) : JNI_FALSE;
}

// -- Primitives --------------------------------------------------------------

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_addPrimitive(JNIEnv* env, jclass, jint type, jfloatArray transform, jfloat r, jfloat g, jfloat b, jfloat a) {
    if (!g_engine) return -1;
    
    jfloat* tElements = env->GetFloatArrayElements(transform, nullptr);
    feather::Mat4 mat = glm::make_mat4(tElements);
    env->ReleaseFloatArrayElements(transform, tElements, JNI_ABORT);
    
    return g_engine->addPrimitive(static_cast<feather::PrimitiveType>(type), mat, feather::Vec4(r, g, b, a));
}

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_getPrimitiveCount(JNIEnv* env, jclass) {
    if (!g_engine) return 0;
    return g_engine->getPrimitiveCount();
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_removePrimitive(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return;
    g_engine->removePrimitive(index);
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_clearPrimitives(JNIEnv* env, jclass) {
    if (!g_engine) return;
    g_engine->clearPrimitives();
}

JNIEXPORT jfloatArray JNICALL
Java_com_feather3d_app_NativeBridge_getPrimitiveMeshVertices(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return nullptr;
    const auto* prim = g_engine->getPrimitive(index);
    if (!prim) return nullptr;
    return meshVerticesToFloatArray(env, prim->mesh);
}

JNIEXPORT jintArray JNICALL
Java_com_feather3d_app_NativeBridge_getPrimitiveMeshIndices(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return nullptr;
    const auto* prim = g_engine->getPrimitive(index);
    if (!prim) return nullptr;
    return meshIndicesToIntArray(env, prim->mesh);
}

JNIEXPORT jfloatArray JNICALL
Java_com_feather3d_app_NativeBridge_getPrimitiveTransform(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return nullptr;
    const auto* prim = g_engine->getPrimitive(index);
    if (!prim) return nullptr;
    
    jfloatArray result = env->NewFloatArray(16);
    if (!result) return nullptr;
    env->SetFloatArrayRegion(result, 0, 16, glm::value_ptr(prim->transform));
    return result;
}

JNIEXPORT jfloatArray JNICALL
Java_com_feather3d_app_NativeBridge_getPrimitiveColor(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return nullptr;
    const auto* prim = g_engine->getPrimitive(index);
    if (!prim) return nullptr;
    
    jfloatArray result = env->NewFloatArray(4);
    if (!result) return nullptr;
    env->SetFloatArrayRegion(result, 0, 4, glm::value_ptr(prim->color));
    return result;
}

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_getPrimitiveType(JNIEnv* env, jclass, jint index) {
    if (!g_engine) return -1;
    const auto* prim = g_engine->getPrimitive(index);
    if (!prim) return -1;
    return prim->primitiveType;
}

// -- Selection & Transform ---------------------------------------------------

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_pickObjectAt(JNIEnv* env, jclass,
    jfloat rayOx, jfloat rayOy, jfloat rayOz,
    jfloat rayDx, jfloat rayDy, jfloat rayDz) {
    if (!g_engine) return -1;
    return g_engine->pickObjectAt(rayOx, rayOy, rayOz, rayDx, rayDy, rayDz);
}

JNIEXPORT jint JNICALL
Java_com_feather3d_app_NativeBridge_getSelectedObjectId(JNIEnv* env, jclass) {
    if (!g_engine) return -1;
    return g_engine->getSelectedObjectId();
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_deselectAll(JNIEnv* env, jclass) {
    if (!g_engine) return;
    g_engine->deselectAll();
}

JNIEXPORT void JNICALL
Java_com_feather3d_app_NativeBridge_transformPrimitive(JNIEnv* env, jclass, jint index, jfloatArray transform) {
    if (!g_engine) return;
    jfloat* tElements = env->GetFloatArrayElements(transform, nullptr);
    feather::Mat4 mat = glm::make_mat4(tElements);
    env->ReleaseFloatArrayElements(transform, tElements, JNI_ABORT);
    g_engine->transformPrimitive(index, mat);
}

} // extern "C"

