package com.feather3d.app

import android.os.Bundle
import android.view.View
import android.view.WindowManager
import android.widget.ImageButton
import android.widget.SeekBar
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import com.feather3d.app.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var featherView: Feather3DView
    private lateinit var exportManager: ExportManager
    private lateinit var sceneManager: SceneManager

    private var currentMode = NativeBridge.MODE_STROKE
    private var currentSubMode = NativeBridge.MODE_STROKE
    private var activeToolButton: ImageButton? = null
    private var activeSubButton: View? = null

    // Tool mode categories
    companion object {
        private const val TOOL_NAVIGATE = -1
        private const val TOOL_DRAW = 0
        private const val TOOL_SCULPT = 1
        private const val TOOL_ERASE = 2
        private const val TOOL_LIQUIFY = 3
        private const val TOOL_PRIMITIVES = 4
        private const val TOOL_SELECT = 5
    }
    private var activeToolCategory = TOOL_DRAW

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Immersive fullscreen — hide system bars
        WindowCompat.setDecorFitsSystemWindows(window, false)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Go edge-to-edge immersive
        WindowInsetsControllerCompat(window, binding.root).let { controller ->
            controller.hide(WindowInsetsCompat.Type.systemBars())
            controller.systemBarsBehavior =
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        // Initialize native engine
        NativeBridge.nativeInit()

        featherView = binding.feather3dView
        featherView.init()

        exportManager = ExportManager(this)
        sceneManager = SceneManager(this)

        // Initialize voxel grid for sculpting (centered, 64³)
        NativeBridge.initVoxelGrid(
            64,
            -0.5f,
            0.5f
        )

        setupSystemMenu()
        setupToolMenu()
        setupBrushSidebar()
        setupHistoryPanel()
        setupContextMenu()
        setupColorPalette()
        setupDraftingMenu()
        setupStats()

        // Default: Draw mode active
        selectTool(TOOL_DRAW, binding.btnModeDraw)
    }

    // ── Panel G: Drafting Menu (Right) ───────────────────────────────────

    private var isStraightLineMode = false
    private var isGridSnap = false
    private var isAngleSnap = false

    private fun setupDraftingMenu() {
        binding.btnStraight.setOnClickListener {
            isStraightLineMode = !isStraightLineMode
            NativeBridge.setStraightLineMode(isStraightLineMode)
            updateDraftingButton(binding.btnStraight, isStraightLineMode)
        }
        binding.btnGridSnap.setOnClickListener {
            isGridSnap = !isGridSnap
            NativeBridge.setGridSnap(isGridSnap, 1.0f) // 1m grid
            updateDraftingButton(binding.btnGridSnap, isGridSnap)
        }
        binding.btnAngleSnap.setOnClickListener {
            isAngleSnap = !isAngleSnap
            NativeBridge.setAngleSnap(isAngleSnap, 15.0f) // 15 degree angle
            updateDraftingButton(binding.btnAngleSnap, isAngleSnap)
        }

        binding.btnSnapView.setOnClickListener {
            featherView.cameraController.snapToNearestView()
        }

        binding.btnOrtho.setOnClickListener {
            val isOrthoNow = !featherView.filamentRenderer.isOrthographic
            featherView.filamentRenderer.isOrthographic = isOrthoNow
            
            if (isOrthoNow) {
                binding.btnOrtho.text = "2D"
                binding.btnOrtho.setTextColor(android.graphics.Color.parseColor("#7C4DFF"))
                binding.btnOrtho.setBackgroundColor(android.graphics.Color.parseColor("#1A7C4DFF"))
            } else {
                binding.btnOrtho.text = "3D"
                binding.btnOrtho.setTextColor(android.graphics.Color.parseColor("#616161"))
                binding.btnOrtho.setBackgroundColor(android.graphics.Color.TRANSPARENT)
            }
        }

        featherView.onDimensionUpdate = { length ->
            if (length < 0f) {
                binding.textDimension.visibility = View.GONE
            } else {
                binding.textDimension.visibility = View.VISIBLE
                binding.textDimension.text = String.format("%.2fm", length)
            }
        }
    }

    private fun updateDraftingButton(btn: com.google.android.material.button.MaterialButton, isActive: Boolean) {
        if (isActive) {
            btn.setTextColor(android.graphics.Color.parseColor("#7C4DFF"))
            btn.setBackgroundColor(android.graphics.Color.parseColor("#1A7C4DFF"))
        } else {
            btn.setTextColor(android.graphics.Color.parseColor("#616161"))
            btn.setBackgroundColor(android.graphics.Color.TRANSPARENT)
        }
    }

    // ── Panel F: Color Palette (Bottom-Right) ────────────────────────────

    private var activeColorView: View? = null
    private var activeColorR = 0.20f
    private var activeColorG = 0.20f
    private var activeColorB = 0.20f

    private fun setupColorPalette() {
        data class ColorEntry(val viewId: Int, val r: Float, val g: Float, val b: Float)

        val colors = listOf(
            ColorEntry(R.id.colorWhite,  1.00f, 1.00f, 1.00f),
            ColorEntry(R.id.colorBlack,  0.20f, 0.20f, 0.20f),
            ColorEntry(R.id.colorRed,    0.90f, 0.22f, 0.21f),
            ColorEntry(R.id.colorOrange, 0.98f, 0.55f, 0.00f),
            ColorEntry(R.id.colorYellow, 0.99f, 0.85f, 0.21f),
            ColorEntry(R.id.colorGreen,  0.26f, 0.63f, 0.28f),
            ColorEntry(R.id.colorCyan,   0.00f, 0.74f, 0.83f),
            ColorEntry(R.id.colorBlue,   0.12f, 0.53f, 0.90f),
            ColorEntry(R.id.colorPurple, 0.49f, 0.30f, 1.00f),
            ColorEntry(R.id.colorPink,   0.93f, 0.25f, 0.48f)
        )

        for (entry in colors) {
            val view = findViewById<View>(entry.viewId)
            view.setOnClickListener {
                NativeBridge.setStrokeColor(entry.r, entry.g, entry.b, 1.0f)
                activeColorR = entry.r
                activeColorG = entry.g
                activeColorB = entry.b

                // Visual feedback: scale up active, scale down previous
                activeColorView?.animate()?.scaleX(1.0f)?.scaleY(1.0f)?.setDuration(100)?.start()
                it.animate()?.scaleX(1.3f)?.scaleY(1.3f)?.setDuration(100)?.start()
                activeColorView = it
            }
        }

        // Default: start with black stroke on white canvas
        NativeBridge.setStrokeColor(0.20f, 0.20f, 0.20f, 1.0f)
        val defaultSwatch = findViewById<View>(R.id.colorBlack)
        defaultSwatch.scaleX = 1.3f
        defaultSwatch.scaleY = 1.3f
        activeColorView = defaultSwatch
    }

    // ── Panel A: System Menu (Top-Left) ──────────────────────────────────

    private fun setupSystemMenu() {
        binding.btnSettings.setOnClickListener {
            showSettingsDialog()
        }

        binding.btnExport.setOnClickListener {
            showFileMenuDialog()
        }

        binding.btnObjectList.setOnClickListener {
            val panel = binding.panelObjectList
            if (panel.visibility == View.VISIBLE) {
                panel.visibility = View.GONE
            } else {
                refreshSceneObjectList()
                panel.visibility = View.VISIBLE
            }
        }
    }

    private fun refreshSceneObjectList() {
        val container = binding.objectListContainer
        container.removeAllViews()

        val primitiveNames = arrayOf("Cube", "Sphere", "Cylinder", "Cone", "Plane", "Torus")

        // Strokes
        val strokeCount = NativeBridge.getStrokeCount()
        for (i in 0 until strokeCount) {
            val tv = android.widget.TextView(this).apply {
                text = "\uD83D\uDD8C Stroke $i"
                textSize = 11f
                setTextColor(0xFF757575.toInt())
                setPadding(8, 4, 8, 4)
            }
            container.addView(tv)
        }

        // Primitives
        val primCount = NativeBridge.getPrimitiveCount()
        for (i in 0 until primCount) {
            val color = NativeBridge.getPrimitiveColor(i)
            val colorHex = if (color != null) {
                val r = (color[0] * 255).toInt().coerceIn(0, 255)
                val g = (color[1] * 255).toInt().coerceIn(0, 255)
                val b = (color[2] * 255).toInt().coerceIn(0, 255)
                String.format("#%02X%02X%02X", r, g, b)
            } else "#888"

            val tv = android.widget.TextView(this).apply {
                text = "\u25A0 ${primitiveNames.getOrElse(0) { "Prim" }} $i"
                textSize = 11f
                setTextColor(android.graphics.Color.parseColor(colorHex))
                setPadding(8, 6, 8, 6)
                setOnClickListener {
                    // Select this primitive
                    featherView.filamentRenderer.highlightPrimitive(i)
                }
            }
            container.addView(tv)
        }

        if (strokeCount == 0 && primCount == 0) {
            val tv = android.widget.TextView(this).apply {
                text = "Empty scene"
                textSize = 11f
                setTextColor(0xFF9E9E9E.toInt())
                setPadding(8, 8, 8, 8)
            }
            container.addView(tv)
        }
    }

    // ── Panel B: Tool Menu (Top-Right) ───────────────────────────────────

    private fun setupToolMenu() {
        binding.btnModeNavigate.setOnClickListener {
            selectTool(TOOL_NAVIGATE, it as ImageButton)
        }
        binding.btnModeDraw.setOnClickListener {
            selectTool(TOOL_DRAW, it as ImageButton)
        }
        binding.btnModeSculpt.setOnClickListener {
            selectTool(TOOL_SCULPT, it as ImageButton)
        }
        binding.btnModeErase.setOnClickListener {
            selectTool(TOOL_ERASE, it as ImageButton)
        }
        binding.btnModeLiquify.setOnClickListener {
            selectTool(TOOL_LIQUIFY, it as ImageButton)
        }
        binding.btnModePrimitives.setOnClickListener {
            selectTool(TOOL_PRIMITIVES, it as ImageButton)
        }
        binding.btnModeSelect.setOnClickListener {
            selectTool(TOOL_SELECT, it as ImageButton)
        }
        binding.btnMirror.setOnClickListener {
            Toast.makeText(this, "Mirror (coming soon)", Toast.LENGTH_SHORT).show()
        }
    }

    private fun selectTool(toolCategory: Int, button: ImageButton) {
        activeToolCategory = toolCategory

        // Reset previous tool visual
        activeToolButton?.setBackgroundResource(R.drawable.bg_tool_btn)
        // Highlight new tool
        button.setBackgroundResource(R.drawable.bg_tool_btn_active)
        activeToolButton = button

        // Set the default sub-mode for each tool category
        when (toolCategory) {
            TOOL_NAVIGATE -> {
                setActiveMode(-1) // -1 disables drawing logic in NativeBridge/StylusInput
                showContextForTool(TOOL_NAVIGATE)
            }
            TOOL_DRAW -> {
                setActiveMode(NativeBridge.MODE_STROKE)
                showContextForTool(TOOL_DRAW)
            }
            TOOL_SCULPT -> {
                setActiveMode(NativeBridge.MODE_SCULPT_ADD)
                showContextForTool(TOOL_SCULPT)
            }
            TOOL_ERASE -> {
                setActiveMode(NativeBridge.MODE_STROKE) // Erase uses stroke removal
                showContextForTool(TOOL_ERASE)
            }
            TOOL_LIQUIFY -> {
                setActiveMode(NativeBridge.MODE_LIQUIFY)
                showContextForTool(TOOL_LIQUIFY)
            }
            TOOL_PRIMITIVES -> {
                setActiveMode(-1)
                showContextForTool(TOOL_PRIMITIVES)
            }
            TOOL_SELECT -> {
                setActiveMode(-2) // Special select mode
                showContextForTool(TOOL_SELECT)
            }
        }
    }

    private fun setActiveMode(mode: Int) {
        currentMode = mode
        currentSubMode = mode
        featherView.setDrawMode(mode)
    }

    // ── Panel C: Brush Sidebar (Left) ────────────────────────────────────

    private fun setupBrushSidebar() {
        binding.brushSizeSlider.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val radius = progress / 1000f  // 0.001 to 0.1
                binding.brushSizeLabel.text = progress.toString()

                NativeBridge.setTubeRadius(radius)
                NativeBridge.setLiquifyRadius(radius * 5f)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

        // Set initial value
        binding.brushSizeSlider.progress = 20

        binding.btnColorPicker.setOnClickListener {
            Toast.makeText(this, "Color Picker (coming soon)", Toast.LENGTH_SHORT).show()
        }
    }

    // ── Panel D: History (Bottom-Left) ───────────────────────────────────

    private fun setupHistoryPanel() {
        binding.btnUndo.setOnClickListener {
            if (NativeBridge.canUndo()) {
                NativeBridge.undo()
                refreshAllMeshes()
                val v = NativeBridge.getTotalVertexCount()
                val t = NativeBridge.getTotalTriangleCount()
                binding.statsText.text = "V: $v | T: $t"
            }
        }

        binding.btnRedo.setOnClickListener {
            if (NativeBridge.canRedo()) {
                NativeBridge.redo()
                refreshAllMeshes()
                val v = NativeBridge.getTotalVertexCount()
                val t = NativeBridge.getTotalTriangleCount()
                binding.statsText.text = "V: $v | T: $t"
            }
        }
    }

    // ── Panel E: Context Menu (Bottom-Center, dynamic) ───────────────────

    private fun setupContextMenu() {
        // Sculpt sub-modes
        binding.btnSubAdd.setOnClickListener {
            setSubMode(NativeBridge.MODE_SCULPT_ADD, it)
        }
        binding.btnSubSub.setOnClickListener {
            setSubMode(NativeBridge.MODE_SCULPT_SUB, it)
        }
        binding.btnSubSmooth.setOnClickListener {
            setSubMode(NativeBridge.MODE_SCULPT_SMOOTH, it)
        }
        binding.btnSubInflate.setOnClickListener {
            setSubMode(NativeBridge.MODE_LIQUIFY_INFLATE, it)
        }
        binding.btnSubPinch.setOnClickListener {
            setSubMode(NativeBridge.MODE_LIQUIFY_PINCH, it)
        }

        binding.btnSubPrimCube.setOnClickListener { featherView.addPrimitive(0, activeColorR, activeColorG, activeColorB) }
        binding.btnSubPrimSphere.setOnClickListener { featherView.addPrimitive(1, activeColorR, activeColorG, activeColorB) }
        binding.btnSubPrimCylinder.setOnClickListener { featherView.addPrimitive(2, activeColorR, activeColorG, activeColorB) }
        binding.btnSubPrimCone.setOnClickListener { featherView.addPrimitive(3, activeColorR, activeColorG, activeColorB) }
        binding.btnSubPrimPlane.setOnClickListener { featherView.addPrimitive(4, activeColorR, activeColorG, activeColorB) }
        binding.btnSubPrimTorus.setOnClickListener { featherView.addPrimitive(5, activeColorR, activeColorG, activeColorB) }

        binding.btnDeleteSelected.setOnClickListener { featherView.deleteSelected() }
        binding.btnDuplicateSelected.setOnClickListener { featherView.duplicateSelected() }


        // Clear
        binding.btnClear.setOnClickListener {
            AlertDialog.Builder(this, com.google.android.material.R.style.ThemeOverlay_MaterialComponents_Dialog_Alert)
                .setTitle("Clear All")
                .setMessage("Remove all strokes and sculpts?")
                .setPositiveButton("Clear") { _, _ ->
                    featherView.clearAll()
                    NativeBridge.initVoxelGrid(64, -0.5f, 0.5f)
                    Toast.makeText(this, "Canvas cleared", Toast.LENGTH_SHORT).show()
                }
                .setNegativeButton("Cancel", null)
                .show()
        }
    }

    private fun setSubMode(mode: Int, button: View) {
        currentSubMode = mode
        featherView.setDrawMode(mode)

        // Update sub-button visuals
        activeSubButton?.let { prev ->
            if (prev is com.google.android.material.button.MaterialButton) {
                prev.setTextColor(0xFF616161.toInt())
                prev.setBackgroundColor(0x00000000)
            }
        }
        if (button is com.google.android.material.button.MaterialButton) {
            button.setTextColor(0xFFFFFFFF.toInt())
            button.setBackgroundColor(0xFF7C4DFF.toInt())
        }
        activeSubButton = button
    }

    /**
     * Show or hide context-menu sub-buttons depending on the active tool.
     */
    private fun showContextForTool(toolCategory: Int) {
        // Reset previous sub-button highlight
        activeSubButton = null

        val add = binding.btnSubAdd
        val sub = binding.btnSubSub
        val smooth = binding.btnSubSmooth
        val inflate = binding.btnSubInflate
        val pinch = binding.btnSubPinch

        val pCube = binding.btnSubPrimCube
        val pSphere = binding.btnSubPrimSphere
        val pCyl = binding.btnSubPrimCylinder
        val pCone = binding.btnSubPrimCone
        val pPlane = binding.btnSubPrimPlane
        val pTorus = binding.btnSubPrimTorus
        val deleteBtn = binding.btnDeleteSelected
        val dupeBtn = binding.btnDuplicateSelected

        // Reset all text colors
        listOf(add, sub, smooth, inflate, pinch, pCube, pSphere, pCyl, pCone, pPlane, pTorus).forEach {
            it.setTextColor(0xFF616161.toInt())
            it.setBackgroundColor(0x00000000)
        }
        deleteBtn.visibility = View.GONE
        dupeBtn.visibility = View.GONE

        when (toolCategory) {
            TOOL_NAVIGATE -> {
                add.visibility = View.GONE
                sub.visibility = View.GONE
                smooth.visibility = View.GONE
                inflate.visibility = View.GONE
                pinch.visibility = View.GONE
                binding.panelContextMenu.visibility = View.GONE
            }
            TOOL_DRAW -> {
                // Show nothing or minimal context
                add.visibility = View.GONE
                sub.visibility = View.GONE
                smooth.visibility = View.GONE
                inflate.visibility = View.GONE
                pinch.visibility = View.GONE
                binding.panelContextMenu.visibility = View.GONE
            }
            TOOL_SCULPT -> {
                binding.panelContextMenu.visibility = View.VISIBLE
                add.visibility = View.VISIBLE
                sub.visibility = View.VISIBLE
                smooth.visibility = View.VISIBLE
                inflate.visibility = View.GONE
                pinch.visibility = View.GONE

                add.text = "Add"
                sub.text = "Sub"
                smooth.text = "Smooth"

                // Default highlight: Add
                setSubMode(NativeBridge.MODE_SCULPT_ADD, add)
            }
            TOOL_ERASE -> {
                binding.panelContextMenu.visibility = View.GONE
                add.visibility = View.GONE
                sub.visibility = View.GONE
                smooth.visibility = View.GONE
                inflate.visibility = View.GONE
                pinch.visibility = View.GONE
            }
            TOOL_LIQUIFY -> {
                binding.panelContextMenu.visibility = View.VISIBLE
                add.visibility = View.GONE
                sub.visibility = View.GONE
                smooth.visibility = View.VISIBLE
                inflate.visibility = View.VISIBLE
                pinch.visibility = View.VISIBLE
                
                pCube.visibility = View.GONE
                pSphere.visibility = View.GONE
                pCyl.visibility = View.GONE
                pCone.visibility = View.GONE
                pPlane.visibility = View.GONE
                pTorus.visibility = View.GONE

                smooth.text = "Smooth"
                inflate.text = "Inflate"
                pinch.text = "Pinch"

                // Default highlight: Inflate
                setSubMode(NativeBridge.MODE_LIQUIFY, smooth)
            }
            TOOL_PRIMITIVES -> {
                binding.panelContextMenu.visibility = View.VISIBLE
                add.visibility = View.GONE
                sub.visibility = View.GONE
                smooth.visibility = View.GONE
                inflate.visibility = View.GONE
                pinch.visibility = View.GONE
                
                pCube.visibility = View.VISIBLE
                pSphere.visibility = View.VISIBLE
                pCyl.visibility = View.VISIBLE
                pCone.visibility = View.VISIBLE
                pPlane.visibility = View.VISIBLE
                pTorus.visibility = View.VISIBLE
            }
            TOOL_SELECT -> {
                binding.panelContextMenu.visibility = View.VISIBLE
                add.visibility = View.GONE
                sub.visibility = View.GONE
                smooth.visibility = View.GONE
                inflate.visibility = View.GONE
                pinch.visibility = View.GONE
                pCube.visibility = View.GONE
                pSphere.visibility = View.GONE
                pCyl.visibility = View.GONE
                pCone.visibility = View.GONE
                pPlane.visibility = View.GONE
                pTorus.visibility = View.GONE
                deleteBtn.visibility = View.VISIBLE
                dupeBtn.visibility = View.VISIBLE
            }
        }
    }

    // ── Stats ────────────────────────────────────────────────────────────

    private fun setupStats() {
        featherView.onStatsUpdate = { vertices, triangles ->
            runOnUiThread {
                binding.statsText.text = "V: $vertices | T: $triangles"
            }
        }
    }

    // ── Export ────────────────────────────────────────────────────────────

    private fun showFileMenuDialog() {
        val options = arrayOf(
            "Save Scene",
            "Load Scene",
            "Delete Scene",
            "Export OBJ",
            "Export GLB (glTF)"
        )
        AlertDialog.Builder(this, com.google.android.material.R.style.ThemeOverlay_MaterialComponents_Dialog_Alert)
            .setTitle("File Menu")
            .setItems(options) { _, which ->
                when (which) {
                    0 -> showSaveSceneDialog()
                    1 -> showLoadSceneDialog()
                    2 -> showDeleteSceneDialog()
                    3 -> exportManager.exportOBJ("skypaint_${System.currentTimeMillis() / 1000}")
                    4 -> exportManager.exportGLB("skypaint_${System.currentTimeMillis() / 1000}")
                }
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    private fun showSaveSceneDialog() {
        val input = android.widget.EditText(this)
        input.hint = "Scene Name"
        AlertDialog.Builder(this)
            .setTitle("Save Scene")
            .setView(input)
            .setPositiveButton("Save") { _, _ ->
                val name = input.text.toString().trim()
                if (name.isNotEmpty()) {
                    if (sceneManager.saveScene(name)) {
                        Toast.makeText(this, "Saved: $name", Toast.LENGTH_SHORT).show()
                    } else {
                        Toast.makeText(this, "Save Failed", Toast.LENGTH_SHORT).show()
                    }
                }
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    private fun showLoadSceneDialog() {
        val scenes = sceneManager.listScenes()
        if (scenes.isEmpty()) {
            Toast.makeText(this, "No saved scenes found", Toast.LENGTH_SHORT).show()
            return
        }
        AlertDialog.Builder(this)
            .setTitle("Load Scene")
            .setItems(scenes.toTypedArray()) { _, which ->
                val name = scenes[which]
                val data = sceneManager.loadScene(name)
                if (data != null) {
                    refreshAllMeshes()
                    Toast.makeText(this, "Loaded: $name", Toast.LENGTH_SHORT).show()
                } else {
                    Toast.makeText(this, "Failed to load", Toast.LENGTH_SHORT).show()
                }
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    private fun showDeleteSceneDialog() {
        val scenes = sceneManager.listScenes()
        if (scenes.isEmpty()) {
            Toast.makeText(this, "No saved scenes found", Toast.LENGTH_SHORT).show()
            return
        }
        AlertDialog.Builder(this)
            .setTitle("Delete Scene")
            .setItems(scenes.toTypedArray()) { _, which ->
                val name = scenes[which]
                if (sceneManager.deleteScene(name)) {
                    Toast.makeText(this, "Deleted: $name", Toast.LENGTH_SHORT).show()
                } else {
                    Toast.makeText(this, "Failed to delete", Toast.LENGTH_SHORT).show()
                }
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    // ── Utility ──────────────────────────────────────────────────────────

    private fun refreshAllMeshes() {
        featherView.filamentRenderer.clearAll()

        val strokeCount = NativeBridge.getStrokeCount()
        for (i in 0 until strokeCount) {
            val verts = NativeBridge.getStrokeMeshVertices(i)
            val indices = NativeBridge.getStrokeMeshIndices(i)
            if (verts != null && indices != null) {
                featherView.filamentRenderer.uploadStrokeMesh(i, verts, indices)
            }
        }

        val primCount = NativeBridge.getPrimitiveCount()
        for (i in 0 until primCount) {
            val verts = NativeBridge.getPrimitiveMeshVertices(i)
            val indices = NativeBridge.getPrimitiveMeshIndices(i)
            val transform = NativeBridge.getPrimitiveTransform(i)
            val color = NativeBridge.getPrimitiveColor(i)
            if (verts != null && indices != null && transform != null && color != null) {
                featherView.filamentRenderer.uploadPrimitiveMesh(i, verts, indices, transform, color)
            }
        }

        val voxelVerts = NativeBridge.getVoxelMeshVertices()
        val voxelIndices = NativeBridge.getVoxelMeshIndices()
        if (voxelVerts != null && voxelIndices != null && voxelVerts.isNotEmpty()) {
            featherView.filamentRenderer.uploadVoxelMesh(voxelVerts, voxelIndices)
        }
    }

    // ── Lifecycle ────────────────────────────────────────────────────────

    override fun onResume() {
        super.onResume()
        featherView.resume()
    }

    override fun onPause() {
        super.onPause()
        featherView.pause()
    }

    override fun onDestroy() {
        featherView.destroy()
        NativeBridge.nativeDestroy()
        super.onDestroy()
    }

    // ── Settings Dialog ─────────────────────────────────────────────────

    private var showGrid = true
    private var showStats = true

    private fun showSettingsDialog() {
        val items = arrayOf(
            "Grid: ${if (showGrid) "ON" else "OFF"}",
            "Wireframe: ${if (featherView.filamentRenderer.wireframeMode) "ON" else "OFF"}",
            "Stats: ${if (showStats) "ON" else "OFF"}",
            "Shading: ${featherView.filamentRenderer.shadingMode.name}"
        )

        AlertDialog.Builder(this, com.google.android.material.R.style.ThemeOverlay_MaterialComponents_Dialog_Alert)
            .setTitle("Settings")
            .setItems(items) { _, which ->
                when (which) {
                    0 -> {
                        showGrid = !showGrid
                        featherView.filamentRenderer.setGridVisible(showGrid)
                        Toast.makeText(this, "Grid: ${if (showGrid) "ON" else "OFF"}", Toast.LENGTH_SHORT).show()
                    }
                    1 -> {
                        featherView.filamentRenderer.wireframeMode = !featherView.filamentRenderer.wireframeMode
                        Toast.makeText(this, "Wireframe: ${if (featherView.filamentRenderer.wireframeMode) "ON" else "OFF"}", Toast.LENGTH_SHORT).show()
                    }
                    2 -> {
                        showStats = !showStats
                        binding.statsText.visibility = if (showStats) View.VISIBLE else View.GONE
                        Toast.makeText(this, "Stats: ${if (showStats) "ON" else "OFF"}", Toast.LENGTH_SHORT).show()
                    }
                    3 -> {
                        // Cycle shading modes
                        val modes = FilamentRenderer.ShadingMode.entries.toTypedArray()
                        val current = featherView.filamentRenderer.shadingMode
                        val nextIdx = (modes.indexOf(current) + 1) % modes.size
                        featherView.filamentRenderer.shadingMode = modes[nextIdx]
                        Toast.makeText(this, "Shading: ${modes[nextIdx].name}", Toast.LENGTH_SHORT).show()
                    }
                }
            }
            .setNegativeButton("Close", null)
            .show()
    }
}
