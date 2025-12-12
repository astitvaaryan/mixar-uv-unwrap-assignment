# MixaR UV Unwrap Assignment — Final Submission

This repository contains the complete implementation of the UV Unwrap assignment:

- **Part 1 — C++ LSCM Unwrap Engine**  
- **Part 2 — Python Processor + CLI + Metrics + Multithreading**  
- **Part 3 — Blender Add-on Integration**

Everything is tested, documented, and runnable.

---

# 🧩 Part 1 — C++ UV Unwrap Engine

Implemented in:

starter_code/part1_cpp/

markdown
Copy code

The engine performs:

- Mesh loading
- Topology analysis
- Seam detection (graph-based)
- LSCM parameterization
- UV island packing
- Stretch & coverage reporting
- OBJ reading/writing

## ✔ Part 1 Requirements Completed

| Requirement | Status |
|------------|--------|
| All tests pass | ✅ 8/8 passed |
| ALGORITHM.md included | ✅ |
| TEST_RESULTS.txt included | ✅ |
| Code compiles cleanly | ✅ |
| Memory leaks checked | ⚠️ **Valgrind not available on Windows — see note** |

### Memory Leak Note (Windows)

Valgrind is **not available natively on Windows**. This is documented here:

> **Valgrind is not supported natively on Windows.**  
> Memory validation was performed using Visual Studio diagnostics (no leaks/invalid reads observed). If you run on Linux, please run the provided valgrind commands in the README or CI.

### Running Part 1 Tests

Inside:

starter_code/part1_cpp/build/

mathematica
Copy code

Run:

```sh
cmake .. -DEIGEN3_INCLUDE_DIR=../third_party/eigen
cmake --build . --config Debug
cd Debug
test_unwrap.exe
Expected output: 8 passed, 0 failed
```
🐍 Part 2 — Python UV Unwrap Processor
Located in:

bash
Copy code
starter_code/part2_python/
This module wraps the C++ library and provides:

Python mesh structure

C++ binding layer

Full CLI (unwrap, batch, optimize)

Batch processing with multithreading

Quality metrics (stretch, coverage, angle distortion)

Parameter optimizer

✔ Part 2 Features
CLI Tool (cli.py)
Unwrap a single mesh

```sh
python cli.py unwrap \
    ../test_data/meshes/01_cube.obj \
    01_cube_unwrapped.obj \
    --angle 30 \
    --min-faces 5 \
    --margin 0.02
```
Batch unwrap (multithreaded)
```sh
python cli.py batch \
    ../test_data/meshes \
    outputs/ \
    --threads 4
```
Optimize parameters
```sh
python cli.py optimize \
    ../test_data/meshes/01_cube.obj \
    --metric stretch
```

### Metrics (implemented)
- **Stretch** — SVD-based per-triangle stretch measurement  
- **Coverage** — UV coverage percentage (rasterized bbox approximation)  
- **Angle distortion** — max per-triangle angle difference between 3D and UV

### Multithreading
- **UnwrapProcessor** uses `concurrent.futures.ThreadPoolExecutor`  
- Thread-safe progress callback (`on_progress`)  
- Per-file metrics collected for each mesh  
- Aggregated batch summary (avg stretch, avg coverage, total time, success/fail counts)

### Bindings (`bindings.py`)
- Loads the compiled C++ shared library (platform-aware: checks `build/Debug` then `build/Release`)  
- Converts C arrays ↔ NumPy arrays for vertices/triangles/uvs  
- Passes unwrap parameters (angle threshold, min island faces, pack flag, margin) to C++  
- Exposes: `load_mesh`, `save_mesh`, `unwrap`, cleanup functions  
- Ensures C memory is freed after conversion
---

🎨 Part 3 — Blender Add-on: UV Unwrap (LSCM)

### ✔ Features implemented
#### Unwrap Operator
- Extracts mesh geometry from the active Blender object
- Calls `uvwrap.bindings.unwrap(...)` to run the C++ LSCM engine
- Applies the returned UVs to the active UV map on the Blender mesh
- Reports number of islands and coverage% in a user message

#### Seam Tools
- **Mark seams** automatically using an angle threshold parameter
- **Clear seams** (remove all seam marks on the mesh)
- Seam edits **invalidate cache** (so stale cached UVs aren't reused)

#### Batch Unwrap
- Unwraps **all mesh objects in the scene**
- Uses Part 2 multithreading backend for performance
- Displays progress in Blender status bar and reports summary

#### Caching System
- Cache directory: `part3_blender/.blend_cache/`
- Cache key: hash(mesh geometry + unwrap parameters)
- Stores generated UVs and metadata to avoid recomputation
- Automatically invalidates cache when geometry or params change
- Includes simple cleanup policy for stale entries

### UI Panel
**Location:** `View3D → Sidebar (N) → UV Unwrap (LSCM)`

Panel contains:
- Slider: **Angle threshold**
- Input: **Min island faces**
- Slider: **Island margin**
- Checkbox: **Pack islands**
- Buttons:
  - **Unwrap**
  - **Mark seams**
  - **Clear seams**
  - **Batch unwrap**
- Mesh info panel:
  - Vertex count
  - Face count
  - UV layer count

---

## Installation (Blender)
1. Zip the add-on folder:
uv_unwrap_addon/

Ensure zip contains `__init__.py`, `operators.py`, `panels.py`, `core/cache.py`, etc.

2. In Blender:
- `Edit → Preferences → Add-ons → Install...`
- Select the ZIP and click **Install**
- Enable **UV Unwrap (LSCM)** in the add-ons list
- Open the N-sidebar → **UV Unwrap (LSCM)** panel

---

## Requirements
- **Blender**: 4.0+ (use bundled Python for add-on)
- **C++ shared library** (built from Part 1) must be available at one of:
- `starter_code/part1_cpp/build/Debug/uvunwrap.dll`
- `starter_code/part1_cpp/build/Release/uvunwrap.dll`

Bindings search order: `build/Debug` → `build/Release`

