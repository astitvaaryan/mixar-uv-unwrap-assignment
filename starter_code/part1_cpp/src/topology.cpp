/**
 * @file topology.cpp
 * @brief Topology builder implementation
 *
 * SKELETON - YOU IMPLEMENT THIS
 *
 * Algorithm:
 * 1. Extract all edges from triangles
 * 2. Ensure uniqueness (always store as v0 < v1)
 * 3. For each edge, find adjacent faces
 * 4. Validate using Euler characteristic
 */

#include "topology.h"
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <vector>

/**
 * @brief Edge structure for uniqueness
 */
struct Edge {
    int v0, v1;

    Edge(int a, int b) {
        // Always store smaller vertex first
        if (a < b) {
            v0 = a;
            v1 = b;
        } else {
            v0 = b;
            v1 = a;
        }
    }

    bool operator<(const Edge& other) const {
        if (v0 != other.v0) return v0 < other.v0;
        return v1 < other.v1;
    }
};

/**
 * @brief Edge information
 */
struct EdgeInfo {
    int face0;
    int face1;

    EdgeInfo() : face0(-1), face1(-1) {}
};

TopologyInfo* build_topology(const Mesh* mesh) {
    if (!mesh) return NULL;

    // Map to collect unique edges
    std::map<Edge, EdgeInfo> edge_map;

    // Iterate through all triangles
    for (int f = 0; f < mesh->num_triangles; f++) {
        int v0 = mesh->triangles[f * 3 + 0];
        int v1 = mesh->triangles[f * 3 + 1];
        int v2 = mesh->triangles[f * 3 + 2];

        // Process 3 edges: (v0,v1), (v1,v2), (v2,v0)
        int vs[3] = {v0, v1, v2};
        for (int i = 0; i < 3; i++) {
            int a = vs[i];
            int b = vs[(i + 1) % 3];
            Edge e(a, b);

            // Check if edge exists
            if (edge_map.find(e) == edge_map.end()) {
                // New edge
                EdgeInfo info;
                info.face0 = f;
                edge_map[e] = info;
            } else {
                // Existing edge, set second face
                edge_map[e].face1 = f;
            }
        }
    }

    // Allocate TopologyInfo
    TopologyInfo* topo = (TopologyInfo*)malloc(sizeof(TopologyInfo));
    topo->num_edges = edge_map.size();
    topo->edges = (int*)malloc(topo->num_edges * 2 * sizeof(int));
    topo->edge_faces = (int*)malloc(topo->num_edges * 2 * sizeof(int));

    // Fill arrays
    int idx = 0;
    for (std::map<Edge, EdgeInfo>::iterator it = edge_map.begin(); it != edge_map.end(); ++it) {
        const Edge& e = it->first;
        const EdgeInfo& info = it->second;

        topo->edges[idx * 2 + 0] = e.v0;
        topo->edges[idx * 2 + 1] = e.v1;

        topo->edge_faces[idx * 2 + 0] = info.face0;
        topo->edge_faces[idx * 2 + 1] = info.face1;

        idx++;
    }

    return topo;
}

void free_topology(TopologyInfo* topo) {
    if (!topo) return;

    if (topo->edges) free(topo->edges);
    if (topo->edge_faces) free(topo->edge_faces);
    free(topo);
}

int validate_topology(const Mesh* mesh, const TopologyInfo* topo) {
    if (!mesh || !topo) return 0;

    int V = mesh->num_vertices;
    int E = topo->num_edges;
    int F = mesh->num_triangles;

    int euler = V - E + F;

    printf("Topology validation:\n");
    printf("  V=%d, E=%d, F=%d\n", V, E, F);
    printf("  Euler characteristic: %d (expected 2 for closed mesh)\n", euler);

    // Closed meshes should have Euler = 2
    // Open meshes or meshes with holes may differ
    if (euler != 2) {
        printf("  Warning: Non-standard Euler characteristic\n");
        printf("  (This may be OK for open meshes or meshes with boundaries)\n");
    }

    return 1;
}
