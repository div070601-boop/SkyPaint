# SkyPaint

SkyPaint is a high-performance open-source Android application that replicates the core functionality of professional 3D drawing and sculpting tools, inspired by apps like "Feather 3D". It provides a rich 3D canvas where stylus strokes generate physical tubular geometry in space, complemented by a fully-featured voxel sculpting, liquify system, and parametric primitives with interactive selection and manipulation.

Built with a modern stack leveraging **Kotlin Coroutines** for asynchronous UI coordination, **Native C++ via JNI** for heavy computational geometry extraction, and the **Google Filament Engine** for state-of-the-art physically based rendering (PBR).

## ✨ Features

- **3D Canvas & Camera Controls**: Smooth single-finger orbiting, dual-finger panning, and pinch-to-zoom capabilities, plus dynamic Orthographic projection and Snap View locks.
- **Stylus 3D Drawing**: Capture raw pressure and tilt data from stylus devices. Translates screen-space continuous stroke data into dynamic 3D spline-based tube geometry.
- **Precision Drafting Tools**: Rubber-band straight lines, real-time 3D grid, and 15-degree angle snapping, topped with a live geometric dimension measurement display.
- **Per-Vertex Vibrant Colors**: Change stroke colors dynamically mid-brush with full Material support.
- **3D Primitives**: Spawn Cubes, Spheres, Cylinders, Cones, Planes, and Toruses directly into the scene at the camera focal point.
- **Selection & Transform**: Tap-to-pick raycasting (ray-AABB intersection) for object selection. Drag to translate, pinch to scale, and two-finger twist to rotate selected primitives.
- **Voxel Sculpting & CSG**: High-performance voxel engine with **Constructive Solid Geometry (CSG)**. Merge primitives into the voxel terrain with Union or Subtraction operations.
- **Liquify Brush**: Dynamically inflate, pinch, push, or smooth local mesh vertices for organic manipulation.
- **Google Filament Renderer**: Real-time rendering equipped with directional lighting, PBR materials, MatCap shading, and antialiasing.
- **Undo/Redo Command System**: Robust C++ Action Stack supporting history for strokes, voxel sculpts, primitive transforms/lifecycle, and CSG merge operations.
- **Exporting Capabilities**: Export your 3D creations (strokes, voxel sculpts, and primitives) directly to `.obj` or `.glb` formats for external use or 3D printing.
- **Asynchronous Architecture**: Lock-free multi-threaded geometry generation guaranteeing 60+ FPS while avoiding UI thread blockage during dense mesh extraction loops.

## 🛠 Architecture

1. **JNI / C++ Core (`GeometryEngine`)**
   - **Spline Generator**: Processes raw, jagged touch coordinates and interpolates them into smooth curves.
   - **TubeMeshGenerator**: Wraps continuous spline frames with variable-radius circular geometry based on stylus pressure.
   - **PrimitiveGenerator**: Generates parametric meshes for Cube, Sphere, Cylinder, Cone, Plane, and Torus with configurable resolution.
   - **RaycastEngine**: Implements ray-AABB intersection testing for interactive 3D object picking and selection.
   - **VoxelGrid & MarchingCubes**: Implements a structured scalar distance field and triangulates the outer shell for volumetric sculpting.
   - **MeshDecimation**: Reduces polygon counts using vertex clustering to prevent GPU starvation.

2. **Kotlin App Layer**
   - **`Feather3DView`**: Connects Android's native `SurfaceView` composition directly to the Filament graphic contexts. Handles selection gestures (tap-to-pick, drag-to-move, pinch-to-scale).
   - **`StylusInputHandler`**: Raycasts 2D screen coordinates into the 3D near-plane world, intercepting high-frequency motion events and routing them asynchronously.
   - **`FilamentRenderer`**: Dispatches raw vertex (`.filamat`) combinations, lighting, environments, and mesh instances straight to hardware buffers.

## 🚀 Building the Project

1. Clone the repository:
   ```bash
   git clone https://github.com/div070601-boop/SkyPaint.git
   ```
2. Open the project directory (`Feather 3d`) in **Android Studio Ladybug** or newer.
3. Allow Gradle to synchronize dependencies.
4. Hit **Run** on any connected Android device or emulator. (Note: A physical device with Stylus support is highly recommended).

*Ensure that CMake and Native NDK are installed via the SDK Manager, as the core drawing engine is compiled via C++ native build tools.*

---
*Created as an open-source technical achievement and exploration into modern mobile 3D interaction semantics.*
