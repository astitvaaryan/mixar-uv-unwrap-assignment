/**
 * @file packing.cpp
 * @brief UV island packing into [0,1]² texture space
 *
 * SKELETON - YOU IMPLEMENT THIS
 *
 * Algorithm: Shelf packing
 * 1. Compute bounding box for each island
 * 2. Sort islands by height (descending)
 * 3. Pack using shelf algorithm
 * 4. Scale to fit [0,1]²
 */

#include "unwrap.h"
#include "math_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <vector>
#include <algorithm>

/**
 * @brief Island bounding box info
 */
struct Island {
    int id;
    float min_u, max_u, min_v, max_v;
    float width, height;
    float target_x, target_y;  // Packed position
    std::vector<int> vertex_indices;
};

void pack_uv_islands(Mesh* mesh,
                     const UnwrapResult* result,
                     float margin) {
    if (!mesh || !result || !mesh->uvs) return;

    if (result->num_islands <= 1) {
        // Single island, already normalized to [0,1]
        return;
    }

    printf("Packing %d islands...\n", result->num_islands);

    // TODO: Implement island packing
    //
    // ALGORITHM:
    //
    // STEP 1: Compute bounding box for each island
    //   - For each island, find min/max U and V
    //   - Compute width and height
    //   - Track which vertices belong to each island
    //
    // STEP 2: Sort islands by height (descending)
    //   - Larger islands first
    //   - Use std::sort with custom comparator
    //
    // STEP 3: Shelf packing
    //   - Create shelves (horizontal rows)
    //   - Try to fit island in current shelf
    //   - If doesn't fit, create new shelf below
    //   - Track target_x, target_y for each island
    //
    // STEP 4: Move islands to packed positions
    //   - For each island:
    //       offset_x = target_x - current_min_u
    //       offset_y = target_y - current_min_v
    //   - Apply offset to all UVs in island
    //
    // STEP 5: Scale everything to fit [0,1]²
    //   - Find max_width, max_height of packed result
    //   - Scale all UVs by 1.0 / max(max_width, max_height)
    //
    // EXPECTED COVERAGE:
    //   - Shelf packing: > 60%
    //
    // BONUS (+10 points):
    //   - Implement MaxRects or Skyline packing for > 75% coverage

    std::vector<Island> islands(result->num_islands);
    int num_islands = result->num_islands;
    
    // Initialize islands
    for (int i = 0; i < num_islands; i++) {
        islands[i].id = i;
        islands[i].min_u = FLT_MAX;
        islands[i].max_u = -FLT_MAX;
        islands[i].min_v = FLT_MAX;
        islands[i].max_v = -FLT_MAX;
        // Vertex indices not strictly needed if we just track UV bounds and offset later
        // But to apply offset, we need to know which vertices to move.
        // Since we have face_island_ids, we can iterate faces later again.
        // OR store vertices here.
        // Storing vertices is safer to avoid repeated updates.
    }
    
    // STEP 1: Compute bounding boxes and collect vertices
    // We assume mesh has split vertices, so each vertex belongs to exactly one island?
    // If so, we can iterate vertices? No, we only have face mapping.
    // Let's iterate faces.
    
    std::vector<int> vert_to_island(mesh->num_vertices, -1);
    
    for (int f = 0; f < mesh->num_triangles; f++) {
        int island_id = result->face_island_ids[f];
        if (island_id < 0 || island_id >= num_islands) continue;
        
        Island& island = islands[island_id];
        
        for (int j = 0; j < 3; j++) {
            int v = mesh->triangles[f * 3 + j];
            
            // Assign vertex to island (first time only)
            if (vert_to_island[v] == -1) {
                vert_to_island[v] = island_id;
                island.vertex_indices.push_back(v);
                
                float u = mesh->uvs[v * 2 + 0];
                float v_coord = mesh->uvs[v * 2 + 1];
                
                island.min_u = min_float(island.min_u, u);
                island.max_u = max_float(island.max_u, u);
                island.min_v = min_float(island.min_v, v_coord);
                island.max_v = max_float(island.max_v, v_coord);
            }
        }
    }
    
    // Normalize dimensions
    for (int i = 0; i < num_islands; i++) {
        Island& isl = islands[i];
        // Check for empty islands
        if (isl.min_u == FLT_MAX) {
            isl.width = 0; 
            isl.height = 0;
            continue;
        }
        isl.width = isl.max_u - isl.min_u;
        isl.height = isl.max_v - isl.min_v;
        
        // Add minimal margin to width/height to ensure spacing?
        // Or apply margin during packing.
        // Applying during packing is safer.
    }

    // STEP 2: Sort by height (descending)
    // We need to sort but keep track of indices. 
    // std::sort moves objects.
    std::sort(islands.begin(), islands.end(), [](const Island& a, const Island& b) {
        return a.height > b.height;
    });

    // STEP 3: Shelf packing
    float map_width = 1.0f; // Target
    float current_x = 0.0f;
    float current_y = 0.0f;
    float shelf_height = 0.0f;
    
    // For final scaling
    float packed_max_w = 0.0f;
    float packed_max_h = 0.0f;

    for (int i = 0; i < num_islands; i++) {
        Island& isl = islands[i];
        if (isl.width == 0) continue;
        
        // Check if fits in current shelf
        if (current_x + isl.width > map_width && current_x > 0) {
            // New shelf
            current_y += shelf_height + margin;
            current_x = 0.0f;
            shelf_height = 0.0f;
        }
        
        // Place here
        isl.target_x = current_x;
        isl.target_y = current_y;
        
        // Update shelf
        current_x += isl.width + margin;
        shelf_height = max_float(shelf_height, isl.height);
        
        packed_max_w = max_float(packed_max_w, isl.target_x + isl.width);
        packed_max_h = max_float(packed_max_h, isl.target_y + isl.height);
    }
    
    // STEP 4: Move islands
    for (int i = 0; i < num_islands; i++) {
        const Island& isl = islands[i];
        if (isl.width == 0) continue;
        
        float offset_x = isl.target_x - isl.min_u;
        float offset_y = isl.target_y - isl.min_v;
        
        for (int v : isl.vertex_indices) {
            mesh->uvs[v * 2 + 0] += offset_x;
            mesh->uvs[v * 2 + 1] += offset_y;
        }
    }

    // STEP 5: Scale to [0,1]
    float scale = 1.0f / max_float(packed_max_w, packed_max_h);
    // Avoid inf
    if (packed_max_w == 0) scale = 1.0f;
    
    for (int i = 0; i < mesh->num_vertices; i++) {
         mesh->uvs[i * 2 + 0] *= scale;
         mesh->uvs[i * 2 + 1] *= scale;
    }

    printf("  Packing completed. Coverage: %.1f%%\n", result->coverage * 100);    
}

void compute_quality_metrics(const Mesh* mesh, UnwrapResult* result) {
    if (!mesh || !result || !mesh->uvs) return;

    // TODO: Implement quality metrics computation
    //
    // NOTE: This function is OPTIONAL for Part 1.
    // You will implement full metrics in Part 2 (Python).
    // For Part 1, you can either:
    //   (A) Leave these as defaults (tests will still pass)
    //   (B) Implement basic estimation for testing
    //
    // ALGORITHM (see reference/algorithms.md and part2_python/reference/metrics_spec.md):
    //
    // STRETCH METRIC (SVD-based):
    //   For each triangle:
    //     1. Build Jacobian matrix J (3x2): maps UV space to 3D space
    //        J = [dp/du, dp/dv] where p is 3D position
    //     2. Compute J^T * J (2x2 Gramian matrix)
    //     3. Find eigenvalues λ1, λ2 of J^T * J
    //     4. Singular values: σ1 = sqrt(λ1), σ2 = sqrt(λ2)
    //     5. Stretch = σ1 / σ2 (ratio of max/min stretching)
    //   Average and max stretch across all triangles
    //
    // COVERAGE METRIC (Rasterization-based):
    //   1. Create 1024x1024 bitmap of [0,1]² UV space
    //   2. Rasterize all UV triangles
    //   3. Coverage = (pixels_filled / total_pixels)
    //   Alternative: Use bounding box as approximation
    //
    // EXPECTED RESULTS:
    //   - Good unwrap: avg_stretch < 1.5, max_stretch < 2.0
    //   - Shelf packing: coverage > 0.60 (60%)
    //   - MaxRects packing: coverage > 0.75 (75%)

    // Default values (replace with your implementation)
    result->avg_stretch = 1.0f;
    result->max_stretch = 1.0f;
    result->coverage = 0.7f;

    printf("Quality metrics: (using defaults - implement for accurate values)\n");
    printf("  Avg stretch: %.2f\n", result->avg_stretch);
    printf("  Max stretch: %.2f\n", result->max_stretch);
    printf("  Coverage: %.1f%%\n", result->coverage * 100);
}
