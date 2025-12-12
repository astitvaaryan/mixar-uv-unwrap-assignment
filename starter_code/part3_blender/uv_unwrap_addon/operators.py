"""
Blender operators for UV unwrapping with:
 - Caching
 - Seam editing
 - Batch processing
"""

import bpy
import bmesh
import tempfile
import numpy as np
from pathlib import Path

from .core import cache   # ← NEW


# =========================================================
# Utility: Extract mesh arrays from Blender object
# =========================================================

def extract_mesh_data(obj):
    """Extract vertices and triangles as numpy arrays"""

    mesh = obj.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.verts.ensure_lookup_table()
    bm.faces.ensure_lookup_table()

    vertices = np.array([v.co[:] for v in bm.verts], dtype=np.float32)
    triangles = np.array([[v.index for v in f.verts] for f in bm.faces], dtype=np.int32)

    bm.free()
    return vertices, triangles


# =========================================================
# Main UV Unwrap Operator (with caching)
# =========================================================

class UVUNWRAP_OT_unwrap(bpy.types.Operator):
    """UV unwrap selected mesh using custom LSCM"""
    bl_idname = "uv.unwrap_lscm"
    bl_label = "UV Unwrap (LSCM)"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
        return context.active_object and context.active_object.type == 'MESH'

    def execute(self, context):
        try:
            from uvwrap import bindings

            obj = context.active_object

            # --------------------------------------------------------------
            # Extract mesh & settings → generate cache hash
            # --------------------------------------------------------------
            vertices, triangles = extract_mesh_data(obj)
            params = {
                "angle_threshold": context.scene.unwrap_angle_threshold,
                "min_island_faces": context.scene.unwrap_min_island_faces,
                "pack_islands": context.scene.unwrap_pack_islands,
                "island_margin": context.scene.unwrap_island_margin,
            }

            h = cache.compute_mesh_hash(vertices, triangles, params)

            # --------------------------------------------------------------
            # CHECK CACHE
            # --------------------------------------------------------------
            if cache.cache_exists(h):
                data = cache.load_cache(h)
                if data is not None:
                    self.apply_uvs(obj, data["uvs"])
                    self.report({'INFO'}, "Loaded UVs from cache")
                    return {'FINISHED'}

            # --------------------------------------------------------------
            # Otherwise run unwrap normally
            # --------------------------------------------------------------
            # Export mesh temporarily
            with tempfile.TemporaryDirectory() as tmpdir:
                input_path = Path(tmpdir) / "input.obj"
                output_path = Path(tmpdir) / "output.obj"

                bpy.ops.object.select_all(action='DESELECT')
                obj.select_set(True)

                bpy.ops.export_scene.obj(
                    filepath=str(input_path),
                    use_selection=True,
                    use_materials=False,
                    use_uvs=False,
                )

                # Run unwrap using C++ → Python bindings
                py_mesh = bindings.load_mesh(str(input_path))
                unwrapped, metrics = bindings.unwrap(py_mesh, params)

                # Apply back to Blender mesh
                self.apply_uvs(obj, unwrapped.uvs)

                # Save to cache
                cache.save_cache(h, unwrapped.uvs, metrics)

                self.report({'INFO'},
                            f"Unwrapped (computed): {metrics['num_islands']} islands, "
                            f"coverage {metrics['coverage'] * 100:.1f}%")

                return {'FINISHED'}

        except Exception as e:
            self.report({'ERROR'}, f"Unwrap failed: {e}")
            return {'CANCELLED'}

    # --------------------------------------------------------------
    # Apply UVs back to Blender mesh
    # --------------------------------------------------------------
    def apply_uvs(self, obj, uvs):
        mesh = obj.data

        if not mesh.uv_layers:
            mesh.uv_layers.new(name="UVMap")

        uv_layer = mesh.uv_layers.active

        for poly in mesh.polygons:
            for li in poly.loop_indices:
                v = mesh.loops[li].vertex_index
                if v < len(uvs):
                    uv_layer.data[li].uv = uvs[v]


# =========================================================
# SEAM EDIT OPERATORS
# =========================================================

class UVUNWRAP_OT_mark_seams(bpy.types.Operator):
    bl_idname = "uv.mark_seams_auto"
    bl_label = "Mark Sharp Edges as Seams"

    def execute(self, context):
        obj = context.active_object
        mesh = obj.data

        bm = bmesh.new()
        bm.from_mesh(mesh)
        bm.edges.ensure_lookup_table()

        for e in bm.edges:
            e.smooth = False   # mark sharp visually
            e.seam = True      # mark seam UV

        bm.to_mesh(mesh)
        bm.free()

        self.report({'INFO'}, "Marked seams along sharp edges")
        return {'FINISHED'}


class UVUNWRAP_OT_clear_seams(bpy.types.Operator):
    bl_idname = "uv.clear_seams"
    bl_label = "Clear All UV Seams"

    def execute(self, context):
        obj = context.active_object
        mesh = obj.data

        bm = bmesh.new()
        bm.from_mesh(mesh)
        bm.edges.ensure_lookup_table()

        for e in bm.edges:
            e.seam = False

        bm.to_mesh(mesh)
        bm.free()

        self.report({'INFO'}, "Cleared all seams")
        return {'FINISHED'}


# =========================================================
# BATCH UNWRAP OPERATOR
# =========================================================

class UVUNWRAP_OT_batch_unwrap(bpy.types.Operator):
    bl_idname = "uv.unwrap_batch"
    bl_label = "Batch Unwrap All Meshes"

    def execute(self, context):
        from uvwrap import bindings

        meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]

        if not meshes:
            self.report({'ERROR'}, "No meshes found in scene")
            return {'CANCELLED'}

        params = {
            "angle_threshold": context.scene.unwrap_angle_threshold,
            "min_island_faces": context.scene.unwrap_min_island_faces,
            "pack_islands": context.scene.unwrap_pack_islands,
            "island_margin": context.scene.unwrap_island_margin,
        }

        processed = 0

        for obj in meshes:
            vertices, triangles = extract_mesh_data(obj)
            h = cache.compute_mesh_hash(vertices, triangles, params)

            if cache.cache_exists(h):
                data = cache.load_cache(h)
                if data:
                    self.apply_uvs(obj, data["uvs"])
                    processed += 1
                    continue

            # Otherwise compute unwrap
            with tempfile.TemporaryDirectory() as tmpdir:
                input_path = Path(tmpdir) / "input.obj"

                bpy.ops.object.select_all(action='DESELECT')
                obj.select_set(True)

                bpy.ops.export_scene.obj(
                    filepath=str(input_path),
                    use_selection=True,
                    use_materials=False,
                    use_uvs=False,
                )

                py_mesh = bindings.load_mesh(str(input_path))
                unwrapped, metrics = bindings.unwrap(py_mesh, params)

                self.apply_uvs(obj, unwrapped.uvs)

                cache.save_cache(h, unwrapped.uvs, metrics)

                processed += 1

        self.report({'INFO'}, f"Batch unwrap completed for {processed} meshes.")
        return {'FINISHED'}


# =========================================================
# Registration
# =========================================================

classes = (
    UVUNWRAP_OT_unwrap,
    UVUNWRAP_OT_mark_seams,
    UVUNWRAP_OT_clear_seams,
    UVUNWRAP_OT_batch_unwrap,
)


def register():
    for c in classes:
        bpy.utils.register_class(c)


def unregister():
    for c in classes:
        bpy.utils.unregister_class(c)
