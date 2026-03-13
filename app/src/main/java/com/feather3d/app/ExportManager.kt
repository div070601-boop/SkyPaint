package com.feather3d.app

import android.content.ContentValues
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import java.io.File
import java.io.FileOutputStream

/**
 * Manages file export using SAF (Storage Access Framework)
 * and MediaStore for saving OBJ and GLB files.
 */
class ExportManager(private val context: Context) {

    /**
     * Export scene as OBJ to app-specific storage, then share
     */
    fun exportOBJ(fileName: String = "feather3d_export") {
        val objString = NativeBridge.exportOBJ()
        if (objString.isEmpty()) {
            Toast.makeText(context, "Nothing to export", Toast.LENGTH_SHORT).show()
            return
        }

        try {
            val fullName = "${fileName}.obj"

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                // Use MediaStore for Android 10+
                val values = ContentValues().apply {
                    put(MediaStore.MediaColumns.DISPLAY_NAME, fullName)
                    put(MediaStore.MediaColumns.MIME_TYPE, "application/obj")
                    put(MediaStore.MediaColumns.RELATIVE_PATH,
                        Environment.DIRECTORY_DOWNLOADS + "/Feather3D")
                }

                val uri = context.contentResolver.insert(
                    MediaStore.Downloads.EXTERNAL_CONTENT_URI, values
                )

                uri?.let {
                    context.contentResolver.openOutputStream(it)?.use { out ->
                        out.write(objString.toByteArray())
                    }
                    Toast.makeText(context,
                        "Exported to Downloads/Feather3D/$fullName",
                        Toast.LENGTH_LONG).show()

                    // Offer to share
                    shareFile(it, "application/obj", fullName)
                }
            } else {
                // Legacy storage
                val dir = File(
                    Environment.getExternalStoragePublicDirectory(
                        Environment.DIRECTORY_DOWNLOADS
                    ), "Feather3D"
                )
                dir.mkdirs()

                val file = File(dir, fullName)
                FileOutputStream(file).use { it.write(objString.toByteArray()) }

                Toast.makeText(context,
                    "Exported to ${file.absolutePath}",
                    Toast.LENGTH_LONG).show()
            }
        } catch (e: Exception) {
            Toast.makeText(context,
                "Export failed: ${e.message}",
                Toast.LENGTH_LONG).show()
        }
    }

    /**
     * Export scene as GLB (glTF binary)
     */
    fun exportGLB(fileName: String = "feather3d_export") {
        val glbData = NativeBridge.exportGLB()
        if (glbData == null || glbData.isEmpty()) {
            Toast.makeText(context, "Nothing to export", Toast.LENGTH_SHORT).show()
            return
        }

        try {
            val fullName = "${fileName}.glb"

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                val values = ContentValues().apply {
                    put(MediaStore.MediaColumns.DISPLAY_NAME, fullName)
                    put(MediaStore.MediaColumns.MIME_TYPE, "model/gltf-binary")
                    put(MediaStore.MediaColumns.RELATIVE_PATH,
                        Environment.DIRECTORY_DOWNLOADS + "/Feather3D")
                }

                val uri = context.contentResolver.insert(
                    MediaStore.Downloads.EXTERNAL_CONTENT_URI, values
                )

                uri?.let {
                    context.contentResolver.openOutputStream(it)?.use { out ->
                        out.write(glbData)
                    }
                    Toast.makeText(context,
                        "Exported to Downloads/Feather3D/$fullName",
                        Toast.LENGTH_LONG).show()

                    shareFile(it, "model/gltf-binary", fullName)
                }
            } else {
                val dir = File(
                    Environment.getExternalStoragePublicDirectory(
                        Environment.DIRECTORY_DOWNLOADS
                    ), "Feather3D"
                )
                dir.mkdirs()

                val file = File(dir, fullName)
                FileOutputStream(file).use { it.write(glbData) }

                Toast.makeText(context,
                    "Exported to ${file.absolutePath}",
                    Toast.LENGTH_LONG).show()
            }
        } catch (e: Exception) {
            Toast.makeText(context,
                "Export failed: ${e.message}",
                Toast.LENGTH_LONG).show()
        }
    }

    private fun shareFile(uri: Uri, mimeType: String, fileName: String) {
        try {
            val shareIntent = Intent(Intent.ACTION_SEND).apply {
                type = mimeType
                putExtra(Intent.EXTRA_STREAM, uri)
                putExtra(Intent.EXTRA_SUBJECT, fileName)
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }
            context.startActivity(
                Intent.createChooser(shareIntent, "Share $fileName")
            )
        } catch (_: Exception) {
            // Share dialog not available, file is already saved
        }
    }
}
