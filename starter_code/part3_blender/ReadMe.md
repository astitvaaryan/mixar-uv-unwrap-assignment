# Part 3 — Blender Add-on: UV Unwrap (LSCM)

This folder contains the complete Blender add-on that integrates:

- ✅ Part 1 — C++ LSCM UV Unwrap Engine  
- ✅ Part 2 — Python bindings & metrics  
- ✅ Part 3 — Full Blender workflow (operators, UI, caching)

The add-on provides a production-style UV unwrapping tool directly inside Blender.

---

## ✔ Implemented Features

### 1. **Unwrap Operator**
- Extracts mesh data directly from Blender.
- Calls Python → C++ unwrap through `uvwrap.bindings.unwrap`.
- Applies generated UVs to the active UV map.
- Displays results including island count & UV coverage.
- No OBJ import/export needed (temp export only inside operator).

---

### 2. **Seam Editing Tools**
- **Mark seams automatically** using angle threshold.
- **Clear all seams**.
- Seam operations **invalidate cached unwraps**.
- Seam workflow integrates with Blender’s native UV tools.

---

### 3. **Batch Unwrap**
- Unwraps **all mesh objects in the scene**.
- Uses multithreading from Part 2.
- Shows progress in Blender’s status bar.
- Saves per-object UVs.

---

### 4. **Caching System**
Located inside `core/cache.py`.

The cache:
- Computes a **hash of mesh geometry + unwrap parameters**.
- Stores results in:  
part3_blender/.blend_cache/

- Prevents recomputation when nothing changed.
- Automatically cleans expired entries.

This significantly improves performance during iteration.

---

### 5. **User Interface Panel**

Available in:

> **View3D → Sidebar (N) → UV Unwrap (LSCM)**

Panel includes:

- Angle Threshold  
- Min Island Faces  
- Island Margin  
- Pack Islands toggle  
- Buttons:
- **Unwrap**
- **Mark Seams**
- **Clear Seams**
- **Batch Unwrap**
- Mesh statistics:
- Vert count
- Face count
- UV layer count

---

## 📦 Installation Instructions

### Step 1 — Create ZIP
Zip the folder:



uv_unwrap_addon/


Make sure the ZIP contains:



uv_unwrap_addon/
│── init.py
│── operators.py
│── panels.py
└── core/
├── cache.py
└── init.py


---

### Step 2 — Install in Blender

1. Open **Blender 4.0+**
2. Go to **Edit → Preferences → Add-ons**
3. Click **Install…**
4. Select your zipped folder
5. Enable **UV Unwrap (LSCM)** in the add-on list

---

## 🧪 Usage

### Unwrapping a Mesh
1. Select a mesh
2. Press **N** to open the sidebar
3. Choose **UV Unwrap (LSCM)**
4. Set your parameters
5. Click **Unwrap**

Blender will:
- Call your C++ unwrap engine
- Compute UVs
- Apply UV mapping to the mesh
- Display island count & stretch metrics

---

## 🔧 Requirements

| Component | Version |
|----------|---------|
| **Blender** | 4.0+ |
| **Python** | Blender’s bundled Python |
| **C++ Library** | Must exist at: `part1_cpp/build/Release/uvunwrap.dll` |

The binding loader (`bindings.py`) automatically detects:

- `build/Debug/uvunwrap.dll`
- `build/Release/uvunwrap.dll`

---


## 📝 Notes

- The add-on calls the same unwrap engine used in CLI & tests.
- Cache mechanism avoids re-running LSCM unnecessarily.
- Works for:
  - Static meshes
  - Multi-mesh scenes
  - Seam editing workflows
- Designed to be clean, modular, and production-ready.
