bl_info = {
    "name": "UV Unwrap (LSCM)",
    "author": "Mixar Assignment",
    "version": (1, 0, 0),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > UV Unwrap",
    "description": "Production-ready UV unwrapping using custom LSCM implementation",
    "category": "UV",
}

import sys
from pathlib import Path

# Add Python bindings to path
addon_dir = Path(__file__).parent
local_uvwrap = addon_dir / "uvwrap"

if str(local_uvwrap) not in sys.path:
    sys.path.insert(0, str(addon_dir))

# Import Blender modules
import bpy
from bpy.props import FloatProperty, IntProperty, BoolProperty

# Import addon modules
from . import operators
from . import panels


def register():
    """Register addon classes and properties"""
    operators.register()
    panels.register()
    
    # Add properties to scene
    bpy.types.Scene.unwrap_angle_threshold = FloatProperty(
        name="Angle Threshold",
        description="Seam detection angle threshold (degrees)",
        default=30.0,
        min=1.0,
        max=90.0,
    )
    
    bpy.types.Scene.unwrap_min_island_faces = IntProperty(
        name="Min Island Faces",
        description="Minimum faces per island",
        default=5,
        min=1,
        max=100,
    )
    
    bpy.types.Scene.unwrap_island_margin = FloatProperty(
        name="Island Margin",
        description="Spacing between UV islands",
        default=0.02,
        min=0.0,
        max=0.1,
    )
    
    bpy.types.Scene.unwrap_pack_islands = BoolProperty(
        name="Pack Islands",
        description="Pack UV islands into texture space",
        default=True,
    )
    
    print("UV Unwrap addon registered")


def unregister():
    """Unregister addon classes and properties"""
    operators.unregister()
    panels.unregister()
    
    # Remove properties
    del bpy.types.Scene.unwrap_angle_threshold
    del bpy.types.Scene.unwrap_min_island_faces
    del bpy.types.Scene.unwrap_island_margin
    del bpy.types.Scene.unwrap_pack_islands
    
    print("UV Unwrap addon unregistered")


if __name__ == "__main__":
    register()
