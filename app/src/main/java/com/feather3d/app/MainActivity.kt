package com.feather3d.app

import android.os.Bundle
import android.view.View
import android.view.WindowManager
import android.widget.SeekBar
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import com.feather3d.app.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var featherView: Feather3DView
    private lateinit var exportManager: ExportManager

    private var currentMode = NativeBridge.MODE_STROKE
    private var activeButton: View? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Immersive mode — keep screen on
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

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

        setupToolbar()
        setupToolButtons()
        setupBrushSlider()
        setupStats()

        // Default: Draw mode
        setActiveMode(NativeBridge.MODE_STROKE, binding.btnModeDraw)
    }

    private fun setupToolbar() {
        binding.btnUndo.setOnClickListener {
            val strokeCount = NativeBridge.getStrokeCount()
            if (strokeCount > 0) {
                NativeBridge.removeStroke(strokeCount - 1)
                // Refresh the renderer — remove last stroke entity
                refreshAllMeshes()
                Toast.makeText(this, "Undo stroke", Toast.LENGTH_SHORT).show()
            }
        }

        binding.btnClear.setOnClickListener {
            AlertDialog.Builder(this, com.google.android.material.R.style.ThemeOverlay_MaterialComponents_Dialog_Alert)
                .setTitle("Clear All")
                .setMessage("Remove all strokes and sculpts?")
                .setPositiveButton("Clear") { _, _ ->
                    featherView.clearAll()
                    // Re-initialize voxel grid
                    NativeBridge.initVoxelGrid(64, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f)
                    Toast.makeText(this, "Canvas cleared", Toast.LENGTH_SHORT).show()
                }
                .setNegativeButton("Cancel", null)
                .show()
        }

        binding.btnExport.setOnClickListener {
            showExportDialog()
        }
    }

    private fun setupToolButtons() {
        val modeMap = mapOf(
            binding.btnModeDraw to NativeBridge.MODE_STROKE,
            binding.btnModeSculptAdd to NativeBridge.MODE_SCULPT_ADD,
            binding.btnModeSculptSub to NativeBridge.MODE_SCULPT_SUB,
            binding.btnModeSmooth to NativeBridge.MODE_SCULPT_SMOOTH,
            binding.btnModeLiquify to NativeBridge.MODE_LIQUIFY,
            binding.btnModeInflate to NativeBridge.MODE_LIQUIFY_INFLATE,
            binding.btnModePinch to NativeBridge.MODE_LIQUIFY_PINCH
        )

        for ((button, mode) in modeMap) {
            button.setOnClickListener {
                setActiveMode(mode, it)
            }
        }
    }

    private fun setActiveMode(mode: Int, button: View) {
        currentMode = mode
        featherView.setDrawMode(mode)

        // Update button visuals
        activeButton?.let { prev ->
            if (prev is com.google.android.material.button.MaterialButton) {
                prev.setBackgroundColor(0x00000000) // transparent
                prev.setTextColor(0xFFE8E8E8.toInt())
                prev.strokeColor = android.content.res.ColorStateList.valueOf(0xFF0F3460.toInt())
            }
        }

        if (button is com.google.android.material.button.MaterialButton) {
            button.setBackgroundColor(0xFFE94560.toInt())
            button.setTextColor(0xFFFFFFFF.toInt())
            button.strokeColor = android.content.res.ColorStateList.valueOf(0xFFE94560.toInt())
        }

        activeButton = button
    }

    private fun setupBrushSlider() {
        binding.brushSizeSlider.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                val radius = progress / 1000f  // 0.001 to 0.1
                binding.brushSizeLabel.text = progress.toString()

                NativeBridge.setTubeRadius(radius)
                NativeBridge.setLiquifyRadius(radius * 5f) // Liquify brush is bigger
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

        // Set initial value
        binding.brushSizeSlider.progress = 20
    }

    private fun setupStats() {
        featherView.onStatsUpdate = { vertices, triangles ->
            runOnUiThread {
                binding.statsText.text = "V: $vertices | T: $triangles"
            }
        }
    }

    private fun showExportDialog() {
        val options = arrayOf("Export OBJ", "Export GLB (glTF)")
        AlertDialog.Builder(this, com.google.android.material.R.style.ThemeOverlay_MaterialComponents_Dialog_Alert)
            .setTitle("Export Scene")
            .setItems(options) { _, which ->
                val timestamp = System.currentTimeMillis() / 1000
                when (which) {
                    0 -> exportManager.exportOBJ("feather3d_$timestamp")
                    1 -> exportManager.exportGLB("feather3d_$timestamp")
                }
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

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

        // Refresh voxel mesh
        val voxelVerts = NativeBridge.getVoxelMeshVertices()
        val voxelIndices = NativeBridge.getVoxelMeshIndices()
        if (voxelVerts != null && voxelIndices != null && voxelVerts.isNotEmpty()) {
            featherView.filamentRenderer.uploadVoxelMesh(voxelVerts, voxelIndices)
        }
    }

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
