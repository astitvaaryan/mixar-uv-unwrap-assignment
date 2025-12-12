Part 3 — Blender Add-on: UV Unwrap (LSCM)

This is a complete Blender add-on that integrates the custom UV unwrapping engine built in Part 1 + Part 2.

✔ Implemented Features

#Unwrap Operator
Reads mesh directly from Blender (no OBJ export/import)
Calls Python bindings (uvwrap.bindings.unwrap)
Writes UVs directly to active UV map
Reports number of islands + coverage%

#Seam Editing Tools
Mark seams automatically by angle threshold
Clear all seams
Seam editing invalidates cache

#Batch Unwrap
Unwrap all meshes in the scene
Displays progress in the status bar

#Caching System
Mesh + parameter hashing
Stores UV outputs in .blend_cache/
Prevents recomputation for identical inputs

UI Panel

Located in:
View3D → Sidebar (N) → UV Unwrap (LSCM)

Panel includes:
Angle threshold slider
Min island faces
Island margin
Pack islands checkbox
Buttons for unwrap, seam tools, batch unwrap
Mesh info (verts, faces, UV maps)

Installation

Zip the folder:

uv_unwrap_addon/


In Blender:

Edit → Preferences → Add-ons → Install...


Enable UV Unwrap (LSCM)

Usage

After installation:

Select any mesh

Press N → Open UV Unwrap (LSCM) tab

Adjust parameters

Click Unwrap

A screenshot example (include this in your submission):

(YOU add the screenshot of the add-on enabled — it is required.)

Requirements

Blender 4.0+ 

The C++ DLL must exist in:
part1_cpp/build/Release/uvunwrap.dll