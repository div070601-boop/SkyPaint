package com.feather3d.app

import android.content.Context
import android.util.Log
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * Manages saving and loading scene data (primitives + stroke meshes) as JSON files.
 */
class SceneManager(private val context: Context) {

    companion object {
        private const val TAG = "SceneManager"
        private const val SCENES_DIR = "scenes"
        private const val FILE_EXT = ".skypaint"
    }

    private fun scenesDir(): File {
        val dir = File(context.filesDir, SCENES_DIR)
        if (!dir.exists()) dir.mkdirs()
        return dir
    }

    /**
     * Save the current scene to a named file.
     * Returns true on success.
     */
    fun saveScene(name: String): Boolean {
        return try {
            val root = JSONObject()
            root.put("version", 1)
            root.put("name", name)
            root.put("timestamp", System.currentTimeMillis())

            // Save primitives
            val primArray = JSONArray()
            val primCount = NativeBridge.getPrimitiveCount()
            for (i in 0 until primCount) {
                val obj = JSONObject()
                obj.put("type", NativeBridge.getPrimitiveType(i))

                val transform = NativeBridge.getPrimitiveTransform(i)
                if (transform != null) {
                    val tArr = JSONArray()
                    for (v in transform) tArr.put(v.toDouble())
                    obj.put("transform", tArr)
                }

                val color = NativeBridge.getPrimitiveColor(i)
                if (color != null) {
                    val cArr = JSONArray()
                    for (v in color) cArr.put(v.toDouble())
                    obj.put("color", cArr)
                }

                primArray.put(obj)
            }
            root.put("primitives", primArray)

            // Save strokes (raw points)
            val strokeArray = JSONArray()
            val strokeCount = NativeBridge.getStrokeCount()
            for (i in 0 until strokeCount) {
                val stroke = JSONObject()
                val points = NativeBridge.getStrokePoints(i)
                if (points != null) {
                    val pArr = JSONArray()
                    for (v in points) pArr.put(v.toDouble())
                    stroke.put("points", pArr)
                }
                strokeArray.put(stroke)
            }
            root.put("strokes", strokeArray)

            // Write to file
            val file = File(scenesDir(), "$name$FILE_EXT")
            file.writeText(root.toString())
            Log.i(TAG, "Saved scene '$name' (${primCount} prims, ${strokeCount} strokes)")
            true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to save scene", e)
            false
        }
    }

    /**
     * Load a scene from a named file.
     * Clears current scene first, then re-creates primitives.
     * Note: Stroke meshes are loaded as raw mesh data and need to be uploaded to the renderer.
     * Returns the loaded JSON object for the caller to process stroke meshes, or null on failure.
     */
    fun loadScene(name: String): SceneData? {
        return try {
            val file = File(scenesDir(), "$name$FILE_EXT")
            if (!file.exists()) {
                Log.w(TAG, "Scene file not found: $name")
                return null
            }

            val json = JSONObject(file.readText())
            val version = json.optInt("version", 1)
            Log.i(TAG, "Loading scene '$name' (version $version)")

            // Clear current scene
            NativeBridge.clearStrokes()
            NativeBridge.clearPrimitives()

            // Load primitives
            val primArray = json.optJSONArray("primitives") ?: JSONArray()
            for (i in 0 until primArray.length()) {
                val obj = primArray.getJSONObject(i)
                val type = obj.getInt("type")

                val tArr = obj.getJSONArray("transform")
                val transform = FloatArray(16) { tArr.getDouble(it).toFloat() }

                val cArr = obj.getJSONArray("color")
                val r = cArr.getDouble(0).toFloat()
                val g = cArr.getDouble(1).toFloat()
                val b = cArr.getDouble(2).toFloat()
                val a = cArr.getDouble(3).toFloat()

                NativeBridge.addPrimitive(type, transform, r, g, b, a)
            }

            // Load stroke data by re-feeding the points into NativeBridge
            val strokeArray = json.optJSONArray("strokes") ?: JSONArray()
            for (i in 0 until strokeArray.length()) {
                val stroke = strokeArray.getJSONObject(i)
                val pArr = stroke.optJSONArray("points")
                if (pArr != null) {
                    NativeBridge.beginStroke()
                    val floatsPerPoint = 10
                    val numPoints = pArr.length() / floatsPerPoint
                    for (p in 0 until numPoints) {
                        val base = p * floatsPerPoint
                        val x = pArr.getDouble(base + 0).toFloat()
                        val y = pArr.getDouble(base + 1).toFloat()
                        val z = pArr.getDouble(base + 2).toFloat()
                        val pressure = pArr.getDouble(base + 3).toFloat()
                        val tilt = pArr.getDouble(base + 4).toFloat()
                        val timestamp = pArr.getDouble(base + 5).toFloat()
                        val r = pArr.getDouble(base + 6).toFloat()
                        val g = pArr.getDouble(base + 7).toFloat()
                        val b = pArr.getDouble(base + 8).toFloat()
                        val a = pArr.getDouble(base + 9).toFloat()

                        NativeBridge.setStrokeColor(r, g, b, a)
                        NativeBridge.addStrokePoint(x, y, z, pressure, tilt, timestamp)
                    }
                    NativeBridge.endStroke()
                }
            }

            Log.i(TAG, "Loaded ${primArray.length()} primitives, ${strokeArray.length()} strokes")
            SceneData(name)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load scene", e)
            null
        }
    }

    /**
     * List all saved scene names.
     */
    fun listScenes(): List<String> {
        return scenesDir().listFiles()
            ?.filter { it.name.endsWith(FILE_EXT) }
            ?.map { it.name.removeSuffix(FILE_EXT) }
            ?.sortedDescending()
            ?: emptyList()
    }

    /**
     * Delete a saved scene.
     */
    fun deleteScene(name: String): Boolean {
        val file = File(scenesDir(), "$name$FILE_EXT")
        return file.delete()
    }

    data class SceneData(val name: String)
}
