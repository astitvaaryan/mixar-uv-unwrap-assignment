"""
Caching system for Blender UV Unwrap Add-on
-------------------------------------------
Stores cached UV unwrap results based on:
 - Mesh vertex positions
 - Mesh topology (triangles)
 - Unwrap parameters

Cache location:
   ~/.cache/uv_unwrap_addon/
"""

import os
import json
import hashlib
from pathlib import Path

def cache_dir():
    """Return the base cache directory"""
    base = Path.home() / ".cache" / "uv_unwrap_addon"
    base.mkdir(parents=True, exist_ok=True)
    return base


def compute_mesh_hash(vertices, triangles, params):
    """
    Create a unique hash for:
      - geometry (verts + faces)
      - unwrap parameters
    """

    h = hashlib.sha256()

    # Flatten and encode vertex data
    h.update(vertices.tobytes())

    # Flatten and encode triangles
    h.update(triangles.tobytes())

    # Encode parameters (sorted for consistency)
    param_str = json.dumps(params, sort_keys=True)
    h.update(param_str.encode("utf-8"))

    return h.hexdigest()

def cache_exists(hash_value):
    """Check if cache entry exists"""
    target = cache_dir() / f"{hash_value}.npz"
    return target.exists()


def load_cache(hash_value):
    """Load cached UV data"""
    filepath = cache_dir() / f"{hash_value}.npz"
    if not filepath.exists():
        return None

    try:
        data = dict(np.load(filepath, allow_pickle=True))
        return data
    except:
        return None


def save_cache(hash_value, uvs, metrics):
    """Save computed UVs + metrics"""
    filepath = cache_dir() / f"{hash_value}.npz"

    np.savez_compressed(
        filepath,
        uvs=uvs,
        metrics=metrics
    )

def clear_cache():
    """Delete all cache files"""
    d = cache_dir()
    for f in d.glob("*.npz"):
        f.unlink()

import numpy as np
