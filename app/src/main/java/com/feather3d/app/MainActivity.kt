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

    private var currentMode = NativeBridge.MODE_STROKE
    private var currentSubMode = NativeBridge.MODE_STROKE
    private var activeToolButton: ImageButton? = null
    private var activeSubButton: View? = null

    // Tool mode categories
    companion object {
        private const val TOOL_DRAW = 0
        private const val TOOL_SCULPT = 1
        private const val TOOL_ERASE = 2
        private const val TOOL_LIQUIFY = 3
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

        // Initialize voxel grid for sculpting (centered, 64³)
        NativeBridge.initVoxelGrid(
            64,
            -0.5f, -0.5f, -0.5f,
            0.5f, 0.5f, 0.5f
        )

        setupSystemMenu()
        setupToolMenu()
        setupBrushSidebar()
        setupHistoryPanel()
        setupContextMenu()
        setupStats()

        // Default: Draw mode active
        selectTool(TOOL_DRAW, binding.btnModeDraw)
    }

    // ── Panel A: System Menu (Top-Left) ──────────────────────────────────

    private fun setupSystemMenu() {
        binding.btnSettings.setOnClickListener {
            Toast.makeText(this, "Settings (coming soon)", Toast.LENGTH_SHORT).show()
        }

        binding.btnExport.setOnClickListener {
            showExportDialog()
        }
    }

    // ── Panel B: Tool Menu (Top-Right) ───────────────────────────────────

    private fun setupToolMenu() {
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
            val strokeCount = NativeBridge.getStrokeCount()
            if (strokeCount > 0) {
                NativeBridge.removeStroke(strokeCount - 1)
                refreshAllMeshes()
                Toast.makeText(this, "Undo", Toast.LENGTH_SHORT).show()
            }
        }

        binding.btnRedo.setOnClickListener {
            Toast.makeText(this, "Redo (coming soon)", Toast.LENGTH_SHORT).show()
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

        // Clear
        binding.btnClear.setOnClickListener {
            AlertDialog.Builder(this, com.google.android.material.R.style.ThemeOverlay_MaterialComponents_Dialog_Alert)
                .setTitle("Clear All")
                .setMessage("Remove all strokes and sculpts?")
                .setPositiveButton("Clear") { _, _ ->
                    featherView.clearAll()
                    NativeBridge.initVoxelGrid(64, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f)
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
                prev.setTextColor(0xFFC0C0E0.toInt())
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

        // Reset all text colors
        listOf(add, sub, smooth, inflate, pinch).forEach {
            it.setTextColor(0xFFC0C0E0.toInt())
            it.setBackgroundColor(0x00000000)
        }

        when (toolCategory) {
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

                smooth.text = "Smooth"
                inflate.text = "Inflate"
                pinch.text = "Pinch"

                // Default highlight: Inflate
                setSubMode(NativeBridge.MODE_LIQUIFY, smooth)
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

    private fun showExportDialog() {
        val options = arrayOf("Export OBJ", "Export GLB (glTF)")
        AlertDialog.Builder(this, com.google.android.material.R.style.ThemeOverlay_MaterialComponents_Dialog_Alert)
            .setTitle("Export Scene")
            .setItems(options) { _, which ->
                val timestamp = System.currentTimeMillis() / 1000
                when (which) {
                    0 -> exportManager.exportOBJ("skypaint_$timestamp")
                    1 -> exportManager.exportGLB("skypaint_$timestamp")
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
}
