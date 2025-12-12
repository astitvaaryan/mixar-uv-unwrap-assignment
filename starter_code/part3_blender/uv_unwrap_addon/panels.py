"""
Blender UI panels for UV unwrapping (with caching, seam tools, batch unwrap)
"""

import bpy
from .core import cache
from .operators import extract_mesh_data


class UVUNWRAP_PT_main(bpy.types.Panel):
    """Main UV unwrap panel"""
    bl_label = "UV Unwrap (LSCM)"
    bl_idname = "UVUNWRAP_PT_main"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "UV Unwrap"

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        obj = context.active_object

        # PARAMETERS
        box = layout.box()
        box.label(text="Parameters", icon='SETTINGS')

        box.prop(scene, "unwrap_angle_threshold")
        box.prop(scene, "unwrap_min_island_faces")
        box.prop(scene, "unwrap_island_margin")
        box.prop(scene, "unwrap_pack_islands")

        layout.separator()

        # UNWRAP BUTTON
        col = layout.column(align=True)
        col.scale_y = 1.4
        col.operator("uv.unwrap_lscm", text="Unwrap Selected", icon='UV')

        layout.separator()

        # SEAM TOOLS
        box = layout.box()
        box.label(text="Seam Tools", icon='EDGESEL')

        row = box.row()
        row.operator("uv.mark_seams_auto", text="Mark Sharp as Seams", icon='MOD_EDGESPLIT')

        row = box.row()
        row.operator("uv.clear_seams", text="Clear All Seams", icon='X')

        layout.separator()

        # BATCH PROCESS
        box = layout.box()
        box.label(text="Batch Processing", icon='FILE_FOLDER')

        box.operator("uv.unwrap_batch", text="Unwrap All Meshes", icon='SEQ_LUMA_WAVEFORM')

        layout.separator()

        # MESH INFO
        box = layout.box()
        box.label(text="Mesh Info", icon='INFO')

        if obj and obj.type == 'MESH':
            mesh = obj.data
            box.label(text=f"Object: {obj.name}")
            box.label(text=f"Vertices: {len(mesh.vertices)}")
            box.label(text=f"Faces: {len(mesh.polygons)}")
            box.label(text=f"UV Layers: {len(mesh.uv_layers)}")
        else:
            box.label(text="No mesh selected", icon='ERROR')

        layout.separator()

        # CACHE STATUS
        box = layout.box()
        box.label(text="Cache Status", icon='FILE_CACHE')

        if obj and obj.type == "MESH":
            verts, tris = extract_mesh_data(obj)
            params = {
                "angle_threshold": scene.unwrap_angle_threshold,
                "min_island_faces": scene.unwrap_min_island_faces,
                "pack_islands": scene.unwrap_pack_islands,
                "island_margin": scene.unwrap_island_margin,
            }

            h = cache.compute_mesh_hash(verts, tris, params)

            if cache.cache_exists(h):
                box.label(text="✔ Cached result available", icon='CHECKMARK')
            else:
                box.label(text="✘ Not cached", icon='CANCEL')
        else:
            box.label(text="No cache (no mesh)", icon='ERROR')

# Registration
classes = (UVUNWRAP_PT_main,)

def register():
    for c in classes:
        bpy.utils.register_class(c)

def unregister():
    for c in classes:
        bpy.utils.unregister_class(c)
