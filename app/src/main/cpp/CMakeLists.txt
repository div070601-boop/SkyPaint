cmake_minimum_required(VERSION 3.22.1)
project(feather3d_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android 15+ 16KB page size support
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=16384")

# GLM (header-only)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Engine sources
set(ENGINE_SOURCES
    engine/GeometryEngine.cpp
    engine/StrokeSystem.cpp
    engine/SplineGenerator.cpp
    engine/TubeMeshGenerator.cpp
    engine/VoxelGrid.cpp
    engine/PrimitiveGenerator.cpp
    engine/RaycastEngine.cpp
    engine/MarchingCubes.cpp
    engine/LiquifyBrush.cpp
    engine/MeshDecimation.cpp
    engine/MeshExporter.cpp
    engine/SnapEngine.cpp
    engine/ActionStack.cpp
    jni_bridge.cpp
)

add_library(feather3d_engine SHARED ${ENGINE_SOURCES})

target_include_directories(feather3d_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/math
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(feather3d_engine
    ${log-lib}
    ${android-lib}
)
