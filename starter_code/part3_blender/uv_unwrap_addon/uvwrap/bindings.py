"""
Python bindings to C++ UV unwrapping library

TEMPLATE - YOU IMPLEMENT

Uses ctypes to wrap the C++ shared library.
Alternative: Use pybind11 for cleaner bindings (bonus points).
"""

import ctypes
import os
from pathlib import Path
import numpy as np

# TODO: Find the compiled shared library
# Look in: ../part1_cpp/build/libuvunwrap.so (Linux)
#          ../part1_cpp/build/libuvunwrap.dylib (macOS)
#          ../part1_cpp/build/Release/uvunwrap.dll (Windows)

def find_library():
    """
    Find the compiled C++ library

    Returns:
        Path to library file
    """
    # Get the directory of this Python file
    this_dir = Path(__file__).parent.absolute()
    
    # Build directory is at ../part1_cpp/build/
    cpp_dir = this_dir.parent.parent / "part1_cpp"
    build_dir = cpp_dir / "build"
    
    # Platform-specific library names
    import sys
    if sys.platform == "linux":
        lib_name = "libuvunwrap.so"
    elif sys.platform == "darwin":
        lib_name = "libuvunwrap.dylib"
    
    elif sys.platform == "win32":
        lib_name = "uvunwrap.dll"

        # Check Debug folder first
        debug_path = build_dir / "Debug" / lib_name
        if debug_path.exists():
            return debug_path

        # Check Release folder
        release_path = build_dir / "Release" / lib_name
        if release_path.exists():
            return release_path


    # elif sys.platform == "win32":
    #     lib_name = "uvunwrap.dll"
    #     # Also check Release build
    #     release_path = build_dir / "Release" / lib_name
    #     if release_path.exists():
    #         return release_path
    else:
        raise RuntimeError(f"Unsupported platform: {sys.platform}")
    
    lib_path = build_dir / lib_name
    
    if not lib_path.exists():
        raise FileNotFoundError(
            f"C++ library not found at {lib_path}.\n"
            f"Please build the C++ library first:\n"
            f"  cd {cpp_dir}\n"
            f"  mkdir -p build && cd build\n"
            f"  cmake .. && make"
        )
    
    return lib_path


# Define C structures matching mesh.h BEFORE loading library
class CMesh(ctypes.Structure):
    """
    Matches the Mesh struct in mesh.h
    """
    _fields_ = [
        ('vertices', ctypes.POINTER(ctypes.c_float)),
        ('num_vertices', ctypes.c_int),
        ('triangles', ctypes.POINTER(ctypes.c_int)),
        ('num_triangles', ctypes.c_int),
        ('uvs', ctypes.POINTER(ctypes.c_float)),
    ]


class CUnwrapParams(ctypes.Structure):
    """
    Matches UnwrapParams struct in unwrap.h
    """
    _fields_ = [
        ('angle_threshold', ctypes.c_float),
        ('min_island_faces', ctypes.c_int),
        ('pack_islands', ctypes.c_int),
        ('island_margin', ctypes.c_float),
    ]


class CUnwrapResult(ctypes.Structure):
    """
    Matches UnwrapResult struct in unwrap.h
    """
    _fields_ = [
        ('num_islands', ctypes.c_int),
        ('face_island_ids', ctypes.POINTER(ctypes.c_int)),
        ('avg_stretch', ctypes.c_float),
        ('max_stretch', ctypes.c_float),
        ('coverage', ctypes.c_float),
    ]


# Load library AFTER defining structs
_lib_path = find_library()
_lib = ctypes.CDLL(str(_lib_path))

# Define function signatures
_lib.load_obj.argtypes = [ctypes.c_char_p]
_lib.load_obj.restype = ctypes.POINTER(CMesh)

_lib.save_obj.argtypes = [ctypes.POINTER(CMesh), ctypes.c_char_p]
_lib.save_obj.restype = ctypes.c_int

_lib.free_mesh.argtypes = [ctypes.POINTER(CMesh)]
_lib.free_mesh.restype = None

_lib.unwrap_mesh.argtypes = [
    ctypes.POINTER(CMesh),
    ctypes.POINTER(CUnwrapParams),
    ctypes.POINTER(ctypes.POINTER(CUnwrapResult))
]
_lib.unwrap_mesh.restype = ctypes.POINTER(CMesh)

_lib.free_unwrap_result.argtypes = [ctypes.POINTER(CUnwrapResult)]
_lib.free_unwrap_result.restype = None


class Mesh:
    """
    Python wrapper for C mesh

    Attributes:
        vertices: numpy array (N, 3) of vertex positions
        triangles: numpy array (M, 3) of triangle indices
        uvs: numpy array (N, 2) of UV coordinates (optional)
    """

    def __init__(self, vertices, triangles, uvs=None):
        self.vertices = np.array(vertices, dtype=np.float32)
        self.triangles = np.array(triangles, dtype=np.int32)
        self.uvs = np.array(uvs, dtype=np.float32) if uvs is not None else None

    @property
    def num_vertices(self):
        return len(self.vertices)

    @property
    def num_triangles(self):
        return len(self.triangles)


def load_mesh(filename):
    """
    Load mesh from OBJ file

    Args:
        filename: Path to OBJ file

    Returns:
        Mesh object
    """
    # Call C library
    c_mesh = _lib.load_obj(str(filename).encode('utf-8'))
    if not c_mesh:
        raise RuntimeError(f"Failed to load mesh from {filename}")
    
    # Convert to Python
    num_verts = c_mesh.contents.num_vertices
    num_tris = c_mesh.contents.num_triangles
    
    # Copy vertices
    verts_flat = np.ctypeslib.as_array(c_mesh.contents.vertices, shape=(num_verts * 3,))
    vertices = verts_flat.reshape(-1, 3).copy()
    
    # Copy triangles
    tris_flat = np.ctypeslib.as_array(c_mesh.contents.triangles, shape=(num_tris * 3,))
    triangles = tris_flat.reshape(-1, 3).copy()
    
    # Copy UVs if present
    uvs = None
    if c_mesh.contents.uvs:
        uvs_flat = np.ctypeslib.as_array(c_mesh.contents.uvs, shape=(num_verts * 2,))
        uvs = uvs_flat.reshape(-1, 2).copy()
    
    # Free C mesh
    _lib.free_mesh(c_mesh)
    
    return Mesh(vertices, triangles, uvs)


def save_mesh(mesh, filename):
    """
    Save mesh to OBJ file

    Args:
        mesh: Mesh object
        filename: Output path
    """
    # Create C mesh
    c_mesh = CMesh()
    c_mesh.num_vertices = mesh.num_vertices
    c_mesh.num_triangles = mesh.num_triangles
    
    # Allocate and copy vertices
    verts_flat = mesh.vertices.flatten().astype(np.float32)
    c_mesh.vertices = verts_flat.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    
    # Allocate and copy triangles
    tris_flat = mesh.triangles.flatten().astype(np.int32)
    c_mesh.triangles = tris_flat.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    
    # Copy UVs if present
    if mesh.uvs is not None:
        uvs_flat = mesh.uvs.flatten().astype(np.float32)
        c_mesh.uvs = uvs_flat.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    else:
        c_mesh.uvs = None
    
    # Call C library
    result = _lib.save_obj(ctypes.byref(c_mesh), str(filename).encode('utf-8'))
    if result != 0:
        raise RuntimeError(f"Failed to save mesh to {filename}")


def unwrap(mesh, params=None):
    """
    Unwrap mesh using LSCM

    Args:
        mesh: Mesh object
        params: Dictionary of parameters

    Returns:
        tuple: (unwrapped_mesh, result_dict)
    """
    # Default parameters
    if params is None:
        params = {}
    
    # Create C parameters
    c_params = CUnwrapParams()
    c_params.angle_threshold = params.get('angle_threshold', 30.0)
    c_params.min_island_faces = params.get('min_island_faces', 5)
    c_params.pack_islands = int(params.get('pack_islands', True))
    c_params.island_margin = params.get('island_margin', 0.02)
    
    # Create C input mesh
    c_mesh_in = CMesh()
    c_mesh_in.num_vertices = mesh.num_vertices
    c_mesh_in.num_triangles = mesh.num_triangles
    
    verts_flat = mesh.vertices.flatten().astype(np.float32)
    tris_flat = mesh.triangles.flatten().astype(np.int32)
    
    c_mesh_in.vertices = verts_flat.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
    c_mesh_in.triangles = tris_flat.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    c_mesh_in.uvs = None
    
    # Call C library
    c_result_ptr = ctypes.POINTER(CUnwrapResult)()
    c_mesh_out = _lib.unwrap_mesh(
        ctypes.byref(c_mesh_in),
        ctypes.byref(c_params),
        ctypes.byref(c_result_ptr)
    )
    
    if not c_mesh_out:
        raise RuntimeError("UV unwrapping failed")
    
    # Convert result mesh
    num_verts = c_mesh_out.contents.num_vertices
    num_tris = c_mesh_out.contents.num_triangles
    
    verts_out = np.ctypeslib.as_array(c_mesh_out.contents.vertices, shape=(num_verts * 3,))
    vertices = verts_out.reshape(-1, 3).copy()
    
    tris_out = np.ctypeslib.as_array(c_mesh_out.contents.triangles, shape=(num_tris * 3,))
    triangles = tris_out.reshape(-1, 3).copy()
    
    uvs_out = np.ctypeslib.as_array(c_mesh_out.contents.uvs, shape=(num_verts * 2,))
    uvs = uvs_out.reshape(-1, 2).copy()
    
    # Extract metrics
    result_dict = {
        'num_islands': c_result_ptr.contents.num_islands,
        'avg_stretch': c_result_ptr.contents.avg_stretch,
        'max_stretch': c_result_ptr.contents.max_stretch,
        'coverage': c_result_ptr.contents.coverage,
    }
    
    # Free C memory
    _lib.free_unwrap_result(c_result_ptr)
    _lib.free_mesh(c_mesh_out)
    
    return Mesh(vertices, triangles, uvs), result_dict


# Example usage (for testing)
if __name__ == "__main__":
    # Test loading
    print("Testing bindings...")

    # TODO: Test with a simple mesh
    # mesh = load_mesh("../../test_data/meshes/01_cube.obj")
    # print(f"Loaded: {mesh.num_vertices} vertices, {mesh.num_triangles} triangles")

    # result_mesh, metrics = unwrap(mesh)
    # print(f"Unwrapped: {metrics['num_islands']} islands")

    pass
