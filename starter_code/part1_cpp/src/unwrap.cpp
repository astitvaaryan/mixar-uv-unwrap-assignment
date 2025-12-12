/**
 * @file unwrap.cpp
 * @brief Main UV unwrapping orchestrator
 *
 * SKELETON - YOU IMPLEMENT THIS
 *
 * This file ties together all the components:
 * - Topology building
 * - Seam detection
 * - Island extraction
 * - LSCM parameterization
 * - Island packing
 */

#include "unwrap.h"
#include "lscm.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <set>
#include <map>

/**
 * @brief Extract UV islands after seam cuts
 *
 * Uses connected components algorithm on face graph after removing seam edges.
 *
 * @param mesh Input mesh
 * @param topo Topology info
 * @param seam_edges Array of seam edge indices
 * @param num_seams Number of seams
 * @param num_islands_out Output: number of islands
 * @return Array of island IDs per face
 */
static int* extract_islands(const Mesh* mesh,
                           const TopologyInfo* topo,
                           const int* seam_edges,
                           int num_seams,
                           int* num_islands_out) {
    // TODO: Implement island extraction
    //
    // Algorithm:
    // 1. Build face adjacency graph (faces connected by non-seam edges)
    // 2. Run connected components (BFS or DFS)
    // 3. Assign island ID to each face
    //
    // Steps:
    // 1. Create std::set<int> of seam edge indices for fast lookup
    // 2. Build adjacency list for faces (only through non-seam edges)
    // 3. Run BFS/DFS to find connected components
    // 4. Return array of island IDs (one per face)

    int* face_island_ids = (int*)malloc(mesh->num_triangles * sizeof(int));

    // Initialize all to -1 (unvisited)
    for (int i = 0; i < mesh->num_triangles; i++) {
        face_island_ids[i] = -1;
    }

    // 1. Create set of seam edge indices for fast lookup
    std::set<int> seam_set;
    for (int i = 0; i < num_seams; i++) {
        seam_set.insert(seam_edges[i]);
    }

    // 2. Build adjacency list for faces (only through non-seam edges)
    // We iterate edges. If edge is not seam and not boundary (2 faces), add adjacency.
    std::vector<std::vector<int>> adj(mesh->num_triangles);
    
    for (int e = 0; e < topo->num_edges; e++) {
        if (seam_set.find(e) != seam_set.end()) continue; // Skip seams
        
        int f0 = topo->edge_faces[e * 2];
        int f1 = topo->edge_faces[e * 2 + 1];
        
        if (f0 != -1 && f1 != -1) {
            adj[f0].push_back(f1);
            adj[f1].push_back(f0);
        }
    }
    
    // 3. Run BFS to find connected components
    int current_island_id = 0;
    
    for (int start_face = 0; start_face < mesh->num_triangles; start_face++) {
        if (face_island_ids[start_face] != -1) continue;
        
        // Start new island
        face_island_ids[start_face] = current_island_id;
        std::vector<int> q;
        q.push_back(start_face);
        
        int head = 0;
        while(head < (int)q.size()) {
            int u = q[head++];
            
            for (int v : adj[u]) {
                if (face_island_ids[v] == -1) {
                    face_island_ids[v] = current_island_id;
                    q.push_back(v);
                }
            }
        }
        current_island_id++;
    }
    
    *num_islands_out = current_island_id;



    printf("Extracted %d UV islands\n", *num_islands_out);

    return face_island_ids;
}

/**
 * @brief Copy UVs from island parameterization to result mesh
 */
static void copy_island_uvs(Mesh* result,
                           const float* island_uvs,
                           const int* face_indices,
                           int num_faces,
                           const std::map<int, int>& global_to_local) {
    // TODO: Implement UV copying
    //
    // For each face in the island:
    //   For each vertex in the face:
    //     global_idx = vertex index in mesh
    //     local_idx = global_to_local[global_idx]
    //     result->uvs[global_idx * 2] = island_uvs[local_idx * 2]
    //     result->uvs[global_idx * 2 + 1] = island_uvs[local_idx * 2 + 1]

    for (int local_f = 0; local_f < num_faces; local_f++) {
        int global_f = face_indices[local_f];
        
        for (int j = 0; j < 3; j++) {
            int global_v = result->triangles[global_f * 3 + j];
            auto it = global_to_local.find(global_v);
            if (it != global_to_local.end()) {
                int local_v = it->second;
                result->uvs[global_v * 2 + 0] = island_uvs[local_v * 2 + 0];
                result->uvs[global_v * 2 + 1] = island_uvs[local_v * 2 + 1];
            }
        }
    }
}

Mesh* unwrap_mesh(const Mesh* mesh,
                  const UnwrapParams* params,
                  UnwrapResult** result_out) {
    if (!mesh || !params || !result_out) {
        fprintf(stderr, "unwrap_mesh: Invalid arguments\n");
        return NULL;
    }

    printf("\n=== UV Unwrapping ===\n");
    printf("Input: %d vertices, %d triangles\n",
           mesh->num_vertices, mesh->num_triangles);
    printf("Parameters:\n");
    printf("  Angle threshold: %.1f°\n", params->angle_threshold);
    printf("  Min island faces: %d\n", params->min_island_faces);
    printf("  Pack islands: %s\n", params->pack_islands ? "yes" : "no");
    printf("  Island margin: %.3f\n", params->island_margin);
    printf("\n");

    // TODO: Implement main unwrapping pipeline
    //
    // STEP 1: Build topology
    TopologyInfo* topo = build_topology(mesh);
    if (!topo) {
        fprintf(stderr, "Failed to build topology\n");
        return NULL;
    }
    validate_topology(mesh, topo);

    // STEP 2: Detect seams
    int num_seams;
    int* seam_edges = detect_seams(mesh, topo, params->angle_threshold, &num_seams);

    // STEP 3: Extract islands
    int num_islands;
    int* face_island_ids = extract_islands(mesh, topo, seam_edges, num_seams, &num_islands);

    // STEP 4: Parameterize each island using LSCM
    Mesh* result = allocate_mesh_copy(mesh);
    result->uvs = (float*)calloc(mesh->num_vertices * 2, sizeof(float));

    for (int island_id = 0; island_id < num_islands; island_id++) {
        printf("\nProcessing island %d/%d...\n", island_id + 1, num_islands);

        // Get faces in this island
        std::vector<int> island_faces;
        for (int f = 0; f < mesh->num_triangles; f++) {
            if (face_island_ids[f] == island_id) {
                island_faces.push_back(f);
            }
        }

        printf("  %d faces in island\n", (int)island_faces.size());

        if (island_faces.size() < params->min_island_faces) {
            printf("  Skipping (too small)\n");
            continue;
        }

        // Build face indices array
        int* face_indices_array = (int*)malloc(island_faces.size() * sizeof(int));
        for(size_t i=0; i<island_faces.size(); i++) face_indices_array[i] = island_faces[i];
        
        // Call LSCM
        float* island_uvs = lscm_parameterize(mesh, face_indices_array, island_faces.size());
        
        if (island_uvs) {
            // Build global_to_local for copying
            std::map<int, int> island_global_to_local;
            for(size_t i=0; i<island_faces.size(); i++) {
                int f = island_faces[i];
                for(int j=0; j<3; j++) {
                    int v = mesh->triangles[f*3+j];
                    if(island_global_to_local.find(v) == island_global_to_local.end()) {
                        int count = island_global_to_local.size();
                        island_global_to_local[v] = count;
                    }
                }
            }
            
            copy_island_uvs(result, island_uvs, face_indices_array, island_faces.size(), island_global_to_local);
            
            free(island_uvs);
        } else {
            fprintf(stderr, "  LSCM failed for island %d\n", island_id);
        }
        
        free(face_indices_array);
    }

    // STEP 5: Pack islands if requested
    if (params->pack_islands) {
        UnwrapResult temp_result;
        temp_result.num_islands = num_islands;
        temp_result.face_island_ids = face_island_ids;

        pack_uv_islands(result, &temp_result, params->island_margin);
    }

    // STEP 6: Compute quality metrics
    UnwrapResult* result_data = (UnwrapResult*)malloc(sizeof(UnwrapResult));
    result_data->num_islands = num_islands;
    result_data->face_island_ids = face_island_ids;
    compute_quality_metrics(result, result_data);

    *result_out = result_data;

    // Cleanup
    free_topology(topo);
    free(seam_edges);

    printf("\n=== Unwrapping Complete ===\n");

    return result;
}

void free_unwrap_result(UnwrapResult* result) {
    if (!result) return;

    if (result->face_island_ids) {
        free(result->face_island_ids);
    }
    free(result);
}
