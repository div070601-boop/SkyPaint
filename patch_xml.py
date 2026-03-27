with open('app/src/main/res/layout/activity_main.xml', 'r', encoding='utf-8') as f:
    content = f.read()

plane_torus_btns = """        <com.google.android.material.button.MaterialButton
            android:id="@+id/btnSubPrimPlane"
            style="@style/Widget.MaterialComponents.Button.TextButton"
            android:layout_width="wrap_content"
            android:layout_height="36dp"
            android:text="Plane"
            android:textSize="12sp"
            android:textColor="#616161"
            android:minWidth="0dp"
            android:paddingHorizontal="14dp"
            android:layout_marginEnd="4dp"
            android:visibility="gone" />\r
\r
        <com.google.android.material.button.MaterialButton
            android:id="@+id/btnSubPrimTorus"
            style="@style/Widget.MaterialComponents.Button.TextButton"
            android:layout_width="wrap_content"
            android:layout_height="36dp"
            android:text="Torus"
            android:textSize="12sp"
            android:textColor="#616161"
            android:minWidth="0dp"
            android:paddingHorizontal="14dp"
            android:layout_marginEnd="4dp"
            android:visibility="gone" />\r
\r
"""

marker = 'android:id="@+id/btnDeleteSelected"'
idx = content.find(marker)
if idx > 0:
    btn_start = content.rfind('<com.google.android.material.button.MaterialButton', 0, idx)
    ws_start = content.rfind('\n', 0, btn_start) + 1
    content = content[:ws_start] + plane_torus_btns + content[ws_start:]
    with open('app/src/main/res/layout/activity_main.xml', 'w', encoding='utf-8') as f:
        f.write(content)
    print('Done - buttons inserted')
else:
    print('Marker not found')
