# Part 2 — Python UV Unwrap Processor

This module contains the full Python-side processing pipeline for UV unwrapping.  
It wraps the C++ LSCM engine (Part 1) and provides a command-line interface, quality metrics, and multithreaded batch processing.

---

## ✅ Features Implemented

### 1. Command Line Interface (`cli.py`)
Supports three fully functional modes:

#### 🔹 `unwrap` — Unwrap a single OBJ
Loads mesh → calls C++ unwrap → writes output OBJ.

#### 🔹 `batch` — Multithreaded batch unwrapping
- Uses `ThreadPoolExecutor`
- Parallel processing
- Live progress updates

#### 🔹 `optimize` — Parameter search
Searches best:
- angle thresholds  
- minimum island faces  
Optimizes either *stretch* or *coverage*.

---

## 2. Quality Metrics (`metrics.py`)

Implemented metrics:

- **Stretch** (SVD-based triangle distortion)
- **UV Coverage %**
- **Angle Distortion**

These metrics are used in:
- `unwrap` mode output summary  
- `batch` mode per-file result  
- `optimizer` search evaluation  

---

## 3. Multithreading (`processor.py`)

`UnwrapProcessor` includes:

- ThreadPoolExecutor worker pool  
- Safe progress callbacks  
- Per-file metrics collection  
- Batch summary statistics  

This allows fast processing of 10–100+ meshes.

---

## 4. Python ↔ C++ Bindings (`bindings.py`)

Bindings implemented using `ctypes`:

- Load OBJ via C++
- Convert C arrays → NumPy arrays
- Convert NumPy arrays → C arrays
- Pass unwrap parameters to C++:
  - angle threshold  
  - min island faces  
  - island margins  
  - pack / no-pack  
- Free memory on both Python and C++ sides

---

## 📁 Directory Structure

