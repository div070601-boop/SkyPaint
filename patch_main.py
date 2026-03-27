import sys

with open('app/src/main/java/com/skypaint/app/MainActivity.kt', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Add TOOL_PRIMITIVES
content = content.replace('        private const val TOOL_LIQUIFY = 3\n    }', '        private const val TOOL_LIQUIFY = 3\n        private const val TOOL_PRIMITIVES = 4\n    }')
content = content.replace('        private const val TOOL_LIQUIFY = 3\r\n    }', '        private const val TOOL_LIQUIFY = 3\r\n        private const val TOOL_PRIMITIVES = 4\r\n    }')

# 2. Add btnModePrimitives listener
part2 = """        binding.btnModePrimitives.setOnClickListener {
            selectTool(TOOL_PRIMITIVES, it as ImageButton)
        }
        binding.btnMirror"""
content = content.replace('        binding.btnMirror', part2)

# 3. Handle TOOL_PRIMITIVES in selectTool 
part3 = """            TOOL_LIQUIFY -> {
                setActiveMode(NativeBridge.MODE_LIQUIFY)
                showContextForTool(TOOL_LIQUIFY)
            }
            TOOL_PRIMITIVES -> {
                setActiveMode(-1)
                showContextForTool(TOOL_PRIMITIVES)
            }"""
content = content.replace("""            TOOL_LIQUIFY -> {
                setActiveMode(NativeBridge.MODE_LIQUIFY)
                showContextForTool(TOOL_LIQUIFY)
            }""", part3)

# 4. Handle Sub-buttons for Primitives
part4 = """        binding.btnSubPinch.setOnClickListener {
            setSubMode(NativeBridge.MODE_LIQUIFY_PINCH, it)
        }

        binding.btnSubPrimCube.setOnClickListener { skyView.addPrimitive(0) }
        binding.btnSubPrimSphere.setOnClickListener { skyView.addPrimitive(1) }
        binding.btnSubPrimCylinder.setOnClickListener { skyView.addPrimitive(2) }
        binding.btnSubPrimCone.setOnClickListener { skyView.addPrimitive(3) }
"""
content = content.replace("""        binding.btnSubPinch.setOnClickListener {
            setSubMode(NativeBridge.MODE_LIQUIFY_PINCH, it)
        }""", part4)

# 5. Handle Context Menu Visibility
part5 = """        val add = binding.btnSubAdd
        val sub = binding.btnSubSub
        val smooth = binding.btnSubSmooth
        val inflate = binding.btnSubInflate
        val pinch = binding.btnSubPinch

        val pCube = binding.btnSubPrimCube
        val pSphere = binding.btnSubPrimSphere
        val pCyl = binding.btnSubPrimCylinder
        val pCone = binding.btnSubPrimCone

        // Reset all text colors
        listOf(add, sub, smooth, inflate, pinch, pCube, pSphere, pCyl, pCone).forEach {
            it.setTextColor(0xFF616161.toInt())
            it.setBackgroundColor(0x00000000)
        }"""
content = content.replace("""        val add = binding.btnSubAdd
        val sub = binding.btnSubSub
        val smooth = binding.btnSubSmooth
        val inflate = binding.btnSubInflate
        val pinch = binding.btnSubPinch

        // Reset all text colors
        listOf(add, sub, smooth, inflate, pinch).forEach {
            it.setTextColor(0xFF616161.toInt())
            it.setBackgroundColor(0x00000000)
        }""", part5)

part6 = """            TOOL_LIQUIFY -> {
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
            }"""
content = content.replace("""            TOOL_LIQUIFY -> {
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
            }""", part6)

part9 = """        val strokeCount = NativeBridge.getStrokeCount()
        for (i in 0 until strokeCount) {
            val verts = NativeBridge.getStrokeMeshVertices(i)
            val indices = NativeBridge.getStrokeMeshIndices(i)
            if (verts != null && indices != null) {
                skyView.filamentRenderer.uploadStrokeMesh(i, verts, indices)
            }
        }

        val primCount = NativeBridge.getPrimitiveCount()
        for (i in 0 until primCount) {
            val verts = NativeBridge.getPrimitiveMeshVertices(i)
            val indices = NativeBridge.getPrimitiveMeshIndices(i)
            val transform = NativeBridge.getPrimitiveTransform(i)
            val color = NativeBridge.getPrimitiveColor(i)
            if (verts != null && indices != null && transform != null && color != null) {
                skyView.filamentRenderer.uploadPrimitiveMesh(i, verts, indices, transform, color)
            }
        }"""
content = content.replace("""        val strokeCount = NativeBridge.getStrokeCount()
        for (i in 0 until strokeCount) {
            val verts = NativeBridge.getStrokeMeshVertices(i)
            val indices = NativeBridge.getStrokeMeshIndices(i)
            if (verts != null && indices != null) {
                skyView.filamentRenderer.uploadStrokeMesh(i, verts, indices)
            }
        }""", part9)

with open('app/src/main/java/com/skypaint/app/MainActivity.kt', 'w', encoding='utf-8') as f:
    f.write(content)
