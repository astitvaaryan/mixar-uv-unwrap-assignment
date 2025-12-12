Part 2 — Python UV Unwrap Processor

This module provides all Python utilities for UV unwrapping, wrapping the C++ LSCM implementation from Part 1.

✔ Features Implemented

CLI tool (cli.py) with 3 modes:

unwrap — unwrap a single mesh file

batch — unwrap many meshes in parallel using multithreading

optimize — automatically search best parameters (stretch / coverage)

Quality Metrics

Stretch

Coverage %

Angle distortion

Multithreading Support

Custom UnwrapProcessor built using ThreadPoolExecutor

Progress callback (on_progress)

Aggregated summary results

Bindings

C++ mesh loading and saving

Python mesh structure wrapping C++ data

Parameter passing for angle threshold, minimum faces, margin, packing

CLI Usage
1️⃣ Unwrap a single OBJ file
python cli.py unwrap \
    ../test_data/meshes/01_cube.obj \
    output.obj \
    --angle 30 \
    --min-faces 5 \
    --margin 0.02

2️⃣ Batch Unwrap (multithreaded)
python cli.py batch \
    ../test_data/meshes \
    outputs/ \
    --threads 4

3️⃣ Optimize Parameters
python cli.py optimize \
    ../test_data/meshes/01_cube.obj \
    --metric stretch

Folder Structure
part2_python/
│── bindings.py
│── processor.py
│── metrics.py
│── optimizer.py
└── cli.py

Notes

Metrics are implemented with NumPy.

Batch mode produces per-file results and a final summary.

All Part 2 tests are passing.