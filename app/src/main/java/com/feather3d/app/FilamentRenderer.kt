package com.feather3d.app

import android.content.Context
import android.view.Surface
import android.view.SurfaceView
import com.google.android.filament.*
import com.google.android.filament.android.DisplayHelper
import com.google.android.filament.android.UiHelper
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.channels.Channels

/**
 * Manages Filament engine, scene, and rendering.
 * Handles material setup and mesh upload.
 */
class FilamentRenderer(private val context: Context) {

    // Filament core objects
    lateinit var engine: Engine
        private set
    lateinit var renderer: Renderer
        private set
    lateinit var scene: Scene
        private set
    lateinit var view: View
        private set
    lateinit var camera: Camera
        private set

    private lateinit var uiHelper: UiHelper
    private lateinit var displayHelper: DisplayHelper
    private var swapChain: SwapChain? = null

    // Default material
    private lateinit var defaultMaterial: Material
    private lateinit var defaultMaterialInstance: MaterialInstance

    // Entities for rendered meshes
    private val strokeEntities = mutableListOf<Int>()
    private val entityBuffers = mutableMapOf<Int, Pair<VertexBuffer, IndexBuffer>>()

    private var voxelEntity: Int = 0
    private var currentStrokeEntity: Int = 0
    private var gridEntity: Int = 0

    // Camera entity
    @Suppress("unused")
    private var cameraEntity: Int = 0

    // Rendering state
    var wireframeMode = false
    var shadingMode = ShadingMode.MATCAP

    enum class ShadingMode {
        MATCAP, CLAY, UNLIT
    }

    // Vertex buffer layout: position(3) + normal(3) + uv(2) = stride 8 floats
    private val VERTEX_STRIDE = 8 * 4 // bytes

    fun init(surfaceView: SurfaceView) {
        Filament.init()
        engine = Engine.create()
        renderer = engine.createRenderer()
        scene = engine.createScene()
        view = engine.createView()

        // Camera setup
        cameraEntity = EntityManager.get().create()
        camera = engine.createCamera(cameraEntity)
        view.camera = camera
        view.scene = scene

        // View settings
        view.isPostProcessingEnabled = true

        // Ambient occlusion
        view.ambientOcclusionOptions = view.ambientOcclusionOptions.apply {
            enabled = true
        }

        // Create default material
        createDefaultMaterial()

        // Setup UI helper for surface management
        uiHelper = UiHelper(UiHelper.ContextErrorPolicy.DONT_CHECK).apply {
            renderCallback = object : UiHelper.RendererCallback {
                override fun onNativeWindowChanged(surface: Surface) {
                    swapChain?.let { engine.destroySwapChain(it) }
                    swapChain = engine.createSwapChain(surface)
                    displayHelper.attach(renderer, surfaceView.display)
                }

                override fun onDetachedFromSurface() {
                    swapChain?.let {
                        engine.destroySwapChain(it)
                        swapChain = null
                    }
                }

                override fun onResized(width: Int, height: Int) {
                    view.viewport = Viewport(0, 0, width, height)
                    val aspect = width.toFloat() / height.toFloat()
                    camera.setProjection(45.0, aspect.toDouble(), 0.01, 100.0,
                        Camera.Fov.VERTICAL)
                }
            }
            attachTo(surfaceView)
        }

        displayHelper = DisplayHelper(context)

        // Setup skybox / environment
        setupLighting()
        setupGrid()
    }

    private fun createDefaultMaterial() {
        // Use Filament's built-in material — a simple lit material
        // In production, you'd load .filamat files from assets
        val materialBuffer = readAssetOrFallback("materials/default.filamat")

        defaultMaterial = if (materialBuffer != null) {
            Material.Builder()
                .payload(materialBuffer, materialBuffer.remaining())
                .build(engine)
        } else {
            // Fallback: create a basic lit material programmatically
            // Filament requires pre-compiled materials, so we'll use the
            // built-in default from the utils package
            createFallbackMaterial()
        }

        defaultMaterialInstance = defaultMaterial.createInstance().apply {
            setParameter("baseColor", Colors.RgbaType.SRGB, 0.7f, 0.75f, 0.8f, 1.0f)
            setParameter("roughness", 0.6f)
            setParameter("metallic", 0.0f)
        }
    }

    private fun createFallbackMaterial(): Material {
        // Use the material from Filament's built-in resources
        // We'll generate a simple .filamat at build time
        // For now, try loading the matcap material
        val buffer = readAssetOrFallback("materials/clay.filamat")
        return if (buffer != null) {
            Material.Builder()
                .payload(buffer, buffer.remaining())
                .build(engine)
        } else {
            // Filament engine will crash natively if built without a payload.
            throw IllegalStateException(
                "Fatal Error: Missing compiled Filament materials! " +
                "You must compile 'default.filamat' or 'clay.filamat' using the Filament 'matc' tool " +
                "and place them in app/src/main/assets/materials/ before running the app."
            )
        }
    }

    private fun readAssetOrFallback(assetPath: String): ByteBuffer? {
        return try {
            val input = context.assets.open(assetPath)
            val bytes = input.readBytes()
            input.close()
            ByteBuffer.allocateDirect(bytes.size).apply {
                order(ByteOrder.nativeOrder())
                put(bytes)
                flip()
            }
        } catch (_: Exception) {
            null
        }
    }

    private fun setupLighting() {
        // Create a directional light (sun-like)
        val lightEntity = EntityManager.get().create()
        LightManager.Builder(LightManager.Type.DIRECTIONAL)
            .color(1.0f, 0.98f, 0.95f)
            .intensity(80000f)
            .direction(0.0f, -1.0f, -0.5f)
            .castShadows(true)
            .build(engine, lightEntity)
        scene.addEntity(lightEntity)

        // Indirect light for ambient
        val secondaryLight = EntityManager.get().create()
        LightManager.Builder(LightManager.Type.DIRECTIONAL)
            .color(0.6f, 0.7f, 0.9f)
            .intensity(25000f)
            .direction(0.5f, 0.5f, 1.0f)
            .castShadows(false)
            .build(engine, secondaryLight)
        scene.addEntity(secondaryLight)

        // Set clear color (white paper background — sketchbook feel)
        view.blendMode = View.BlendMode.OPAQUE
        renderer.clearOptions = Renderer.ClearOptions().apply {
            clear = true
            clearColor = floatArrayOf(0.98f, 0.98f, 0.98f, 1.0f) // #FAFAFA white paper
        }
    }

    private fun setupGrid() {
        // Create a simple ground plane grid (optional visual aid)
        // Skip for now — can be added later as a visual helper
    }

    /**
     * Update camera from CameraController state
     */
    fun updateCamera(eye: FloatArray, target: FloatArray, up: FloatArray) {
        camera.lookAt(
            eye[0].toDouble(), eye[1].toDouble(), eye[2].toDouble(),
            target[0].toDouble(), target[1].toDouble(), target[2].toDouble(),
            up[0].toDouble(), up[1].toDouble(), up[2].toDouble()
        )
    }

    /**
     * Upload mesh data from native engine to Filament.
     * vertexData: interleaved [pos.xyz, normal.xyz, uv.xy] × N
     * indexData: triangle indices
     */
    fun uploadStrokeMesh(strokeIndex: Int, vertexData: FloatArray, indexData: IntArray) {
        if (vertexData.isEmpty() || indexData.isEmpty()) return

        val vertexCount = vertexData.size / 8
        val indexCount = indexData.size

        // Create vertex buffer
        val vertexBuffer = VertexBuffer.Builder()
            .vertexCount(vertexCount)
            .bufferCount(1)
            .attribute(VertexBuffer.VertexAttribute.POSITION, 0,
                VertexBuffer.AttributeType.FLOAT3, 0, VERTEX_STRIDE)
            .attribute(VertexBuffer.VertexAttribute.TANGENTS, 0,
                VertexBuffer.AttributeType.FLOAT3, 12, VERTEX_STRIDE)
            .attribute(VertexBuffer.VertexAttribute.UV0, 0,
                VertexBuffer.AttributeType.FLOAT2, 24, VERTEX_STRIDE)
            .build(engine)

        val vbData = ByteBuffer.allocateDirect(vertexData.size * 4)
            .order(ByteOrder.nativeOrder())
        vbData.asFloatBuffer().put(vertexData)
        vertexBuffer.setBufferAt(engine, 0, vbData)

        // Create index buffer
        val indexBuffer = IndexBuffer.Builder()
            .indexCount(indexCount)
            .bufferType(IndexBuffer.Builder.IndexType.UINT)
            .build(engine)

        val ibData = ByteBuffer.allocateDirect(indexData.size * 4)
            .order(ByteOrder.nativeOrder())
        ibData.asIntBuffer().put(indexData)
        indexBuffer.setBuffer(engine, ibData)

        // Create or update entity
        val entity: Int
        if (strokeIndex < strokeEntities.size) {
            entity = strokeEntities[strokeIndex]
            destroyEntityBuffers(entity)
            scene.removeEntity(entity)
            // Destroy old renderable
            engine.destroyEntity(entity)
            entity.let {
                val newEntity = EntityManager.get().create()
                strokeEntities[strokeIndex] = newEntity
                entityBuffers[newEntity] = Pair(vertexBuffer, indexBuffer)
                buildRenderable(newEntity, vertexBuffer, indexBuffer, indexCount)
            }
        } else {
            entity = EntityManager.get().create()
            strokeEntities.add(entity)
            entityBuffers[entity] = Pair(vertexBuffer, indexBuffer)
            buildRenderable(entity, vertexBuffer, indexBuffer, indexCount)
        }
    }

    /**
     * Upload and display the current in-progress stroke preview
     */
    fun uploadCurrentStrokeMesh(vertexData: FloatArray, indexData: IntArray) {
        if (currentStrokeEntity != 0) {
            scene.removeEntity(currentStrokeEntity)
            destroyEntityBuffers(currentStrokeEntity)
            engine.destroyEntity(currentStrokeEntity)
        }

        if (vertexData.isEmpty() || indexData.isEmpty()) {
            currentStrokeEntity = 0
            return
        }

        val vertexCount = vertexData.size / 8
        val indexCount = indexData.size

        val vertexBuffer = VertexBuffer.Builder()
            .vertexCount(vertexCount)
            .bufferCount(1)
            .attribute(VertexBuffer.VertexAttribute.POSITION, 0,
                VertexBuffer.AttributeType.FLOAT3, 0, VERTEX_STRIDE)
            .attribute(VertexBuffer.VertexAttribute.TANGENTS, 0,
                VertexBuffer.AttributeType.FLOAT3, 12, VERTEX_STRIDE)
            .attribute(VertexBuffer.VertexAttribute.UV0, 0,
                VertexBuffer.AttributeType.FLOAT2, 24, VERTEX_STRIDE)
            .build(engine)

        val vbData = ByteBuffer.allocateDirect(vertexData.size * 4)
            .order(ByteOrder.nativeOrder())
        vbData.asFloatBuffer().put(vertexData)
        vertexBuffer.setBufferAt(engine, 0, vbData)

        val indexBuffer = IndexBuffer.Builder()
            .indexCount(indexCount)
            .bufferType(IndexBuffer.Builder.IndexType.UINT)
            .build(engine)

        val ibData = ByteBuffer.allocateDirect(indexData.size * 4)
            .order(ByteOrder.nativeOrder())
        ibData.asIntBuffer().put(indexData)
        indexBuffer.setBuffer(engine, ibData)

        currentStrokeEntity = EntityManager.get().create()
        entityBuffers[currentStrokeEntity] = Pair(vertexBuffer, indexBuffer)
        buildRenderable(currentStrokeEntity, vertexBuffer, indexBuffer, indexCount)
    }

    /**
     * Upload voxel sculpt mesh
     */
    fun uploadVoxelMesh(vertexData: FloatArray, indexData: IntArray) {
        if (voxelEntity != 0) {
            scene.removeEntity(voxelEntity)
            destroyEntityBuffers(voxelEntity)
            engine.destroyEntity(voxelEntity)
        }

        if (vertexData.isEmpty() || indexData.isEmpty()) {
            voxelEntity = 0
            return
        }

        val vertexCount = vertexData.size / 8
        val indexCount = indexData.size

        val vertexBuffer = VertexBuffer.Builder()
            .vertexCount(vertexCount)
            .bufferCount(1)
            .attribute(VertexBuffer.VertexAttribute.POSITION, 0,
                VertexBuffer.AttributeType.FLOAT3, 0, VERTEX_STRIDE)
            .attribute(VertexBuffer.VertexAttribute.TANGENTS, 0,
                VertexBuffer.AttributeType.FLOAT3, 12, VERTEX_STRIDE)
            .attribute(VertexBuffer.VertexAttribute.UV0, 0,
                VertexBuffer.AttributeType.FLOAT2, 24, VERTEX_STRIDE)
            .build(engine)

        val vbData = ByteBuffer.allocateDirect(vertexData.size * 4)
            .order(ByteOrder.nativeOrder())
        vbData.asFloatBuffer().put(vertexData)
        vertexBuffer.setBufferAt(engine, 0, vbData)

        val indexBuffer = IndexBuffer.Builder()
            .indexCount(indexCount)
            .bufferType(IndexBuffer.Builder.IndexType.UINT)
            .build(engine)

        val ibData = ByteBuffer.allocateDirect(indexData.size * 4)
            .order(ByteOrder.nativeOrder())
        ibData.asIntBuffer().put(indexData)
        indexBuffer.setBuffer(engine, ibData)

        voxelEntity = EntityManager.get().create()
        entityBuffers[voxelEntity] = Pair(vertexBuffer, indexBuffer)
        buildRenderable(voxelEntity, vertexBuffer, indexBuffer, indexCount)
    }

    private fun buildRenderable(entity: Int, vb: VertexBuffer, ib: IndexBuffer, indexCount: Int) {
        RenderableManager.Builder(1)
            .boundingBox(Box(0f, 0f, 0f, 100f, 100f, 100f))
            .geometry(0, RenderableManager.PrimitiveType.TRIANGLES, vb, ib, 0, indexCount)
            .material(0, defaultMaterialInstance)
            .receiveShadows(true)
            .castShadows(true)
            .build(engine, entity)

        scene.addEntity(entity)
    }

    /**
     * Render a frame
     */
    fun render(): Boolean {
        if (!uiHelper.isReadyToRender) return false
        val sc = swapChain ?: return false

        if (renderer.beginFrame(sc, 0L)) {
            renderer.render(view)
            renderer.endFrame()
            return true
        }
        return false
    }

    /**
     * Clear all rendered entities
     */
    fun clearAll() {
        strokeEntities.forEach { entity ->
            scene.removeEntity(entity)
            destroyEntityBuffers(entity)
            engine.destroyEntity(entity)
        }
        strokeEntities.clear()

        if (voxelEntity != 0) {
            scene.removeEntity(voxelEntity)
            destroyEntityBuffers(voxelEntity)
            engine.destroyEntity(voxelEntity)
            voxelEntity = 0
        }

        if (currentStrokeEntity != 0) {
            scene.removeEntity(currentStrokeEntity)
            destroyEntityBuffers(currentStrokeEntity)
            engine.destroyEntity(currentStrokeEntity)
            currentStrokeEntity = 0
        }
    }

    private fun destroyEntityBuffers(entity: Int) {
        entityBuffers.remove(entity)?.let { (vb, ib) ->
            engine.destroyVertexBuffer(vb)
            engine.destroyIndexBuffer(ib)
        }
    }

    fun destroy() {
        clearAll()

        engine.destroyMaterialInstance(defaultMaterialInstance)
        engine.destroyMaterial(defaultMaterial)
        engine.destroyView(view)
        engine.destroyScene(scene)
        engine.destroyRenderer(renderer)
        engine.destroyCameraComponent(cameraEntity)

        uiHelper.detach()

        engine.destroy()
    }
}
