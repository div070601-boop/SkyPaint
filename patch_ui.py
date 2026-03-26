import sys

with open('app/src/main/res/layout/activity_main.xml', 'r', encoding='utf-8') as f:
    content = f.read()

part1 = '''        <!-- Primitives -->
        <ImageButton
            android:id="@+id/btnModePrimitives"
            android:layout_width="40dp"
            android:layout_height="40dp"
            android:src="@drawable/ic_shapes"
            android:background="@drawable/bg_tool_btn"
            android:contentDescription="Primitives"
            app:tint="#424242"
            android:layout_margin="2dp" />

        <!-- Erase -->'''
content = content.replace('        <!-- Erase -->', part1)

part2 = '''        <com.google.android.material.button.MaterialButton
            android:id="@+id/btnSubPrimCube"
            style="@style/Widget.MaterialComponents.Button.TextButton"
            android:layout_width="wrap_content"
            android:layout_height="36dp"
            android:text="Cube"
            android:textSize="12sp"
            android:textColor="#616161"
            android:minWidth="0dp"
            android:paddingHorizontal="14dp"
            android:layout_marginEnd="4dp"
            android:visibility="gone" />

        <com.google.android.material.button.MaterialButton
            android:id="@+id/btnSubPrimSphere"
            style="@style/Widget.MaterialComponents.Button.TextButton"
            android:layout_width="wrap_content"
            android:layout_height="36dp"
            android:text="Sphere"
            android:textSize="12sp"
            android:textColor="#616161"
            android:minWidth="0dp"
            android:paddingHorizontal="14dp"
            android:layout_marginEnd="4dp"
            android:visibility="gone" />

        <com.google.android.material.button.MaterialButton
            android:id="@+id/btnSubPrimCylinder"
            style="@style/Widget.MaterialComponents.Button.TextButton"
            android:layout_width="wrap_content"
            android:layout_height="36dp"
            android:text="Cyl"
            android:textSize="12sp"
            android:textColor="#616161"
            android:minWidth="0dp"
            android:paddingHorizontal="14dp"
            android:layout_marginEnd="4dp"
            android:visibility="gone" />

        <com.google.android.material.button.MaterialButton
            android:id="@+id/btnSubPrimCone"
            style="@style/Widget.MaterialComponents.Button.TextButton"
            android:layout_width="wrap_content"
            android:layout_height="36dp"
            android:text="Cone"
            android:textSize="12sp"
            android:textColor="#616161"
            android:minWidth="0dp"
            android:paddingHorizontal="14dp"
            android:layout_marginEnd="4dp"
            android:visibility="gone" />

    </LinearLayout>'''

# Replace only the last occurrence of </LinearLayout> for the context menu
# The Context Menu is the last LinearLayout in the file.
pos = content.rfind('    </LinearLayout>')
if pos != -1:
    content = content[:pos] + part2 + content[pos + len('    </LinearLayout>'):]

with open('app/src/main/res/layout/activity_main.xml', 'w', encoding='utf-8') as f:
    f.write(content)
