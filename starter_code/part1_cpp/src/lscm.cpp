/**
 * @file lscm.cpp
 * @brief LSCM (Least Squares Conformal Maps) parameterization
 *
 * SKELETON - YOU IMPLEMENT THIS
 *
 * This is the MOST COMPLEX part of the assignment.
 *
 * IMPORTANT:
 * - See reference/lscm_matrix_example.cpp for matrix assembly example
 * - See reference/algorithms.md for mathematical background
 * - Use Eigen library for sparse linear algebra
 *
 * Algorithm:
 * 1. Build local vertex mapping (global → local indices)
 * 2. Assemble LSCM sparse matrix
 * 3. Set boundary conditions (pin 2 vertices)
 * 4. Solve sparse linear system
 * 5. Normalize UVs to [0,1]²
 */

#include "lscm.h"
#include "math_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <map>
#include <vector>
#include <set>

// Eigen library for sparse matrices
#include <Eigen/Sparse>
#include <Eigen/SparseLU>
// Alternative: #include <Eigen/IterativeLinearSolvers>

// Helper to compute triangle area and local coordinates
static double compute_triangle_area(const Vec3& p0, const Vec3& p1, const Vec3& p2) {
    Vec3 e1 = vec3_sub(p1, p0);
    Vec3 e2 = vec3_sub(p2, p0);
    return 0.5 * vec3_length(vec3_cross(e1, e2));
}

int find_boundary_vertices(const Mesh* mesh,
                          const int* face_indices,
                          int num_faces,
                          int** boundary_out) {
    if (!mesh || !face_indices || num_faces == 0) return 0;

    // Count edge recurrence to find boundary edges
    // Key: Edge (v0 < v1), Value: count
    std::map<std::pair<int, int>, int> edge_counts;

    for (int i = 0; i < num_faces; i++) {
        int f = face_indices[i];
        int v0 = mesh->triangles[f*3 + 0];
        int v1 = mesh->triangles[f*3 + 1];
        int v2 = mesh->triangles[f*3 + 2];

        int vs[3] = {v0, v1, v2};
        for (int j = 0; j < 3; j++) {
            int a = vs[j];
            int b = vs[(j+1)%3];
            if (a > b) std::swap(a, b);
            edge_counts[{a, b}]++;
        }
    }

    std::set<int> boundary_verts;
    for (auto const& item : edge_counts) {
        const std::pair<int, int>& edge = item.first;
        int count = item.second;
        if (count == 1) {
            boundary_verts.insert(edge.first);
            boundary_verts.insert(edge.second);
        }
    }

    int num_boundary = boundary_verts.size();
    if (num_boundary > 0) {
        *boundary_out = (int*)malloc(num_boundary * sizeof(int));
        int idx = 0;
        for (int v : boundary_verts) {
            (*boundary_out)[idx++] = v;
        }
    } else {
        *boundary_out = NULL;
    }
    
    return num_boundary;
}

void normalize_uvs_to_unit_square(float* uvs, int num_verts) {
    if (!uvs || num_verts == 0) return;

    // Find bounding box
    float min_u = FLT_MAX, max_u = -FLT_MAX;
    float min_v = FLT_MAX, max_v = -FLT_MAX;

    for (int i = 0; i < num_verts; i++) {
        float u = uvs[i * 2];
        float v = uvs[i * 2 + 1];

        min_u = min_float(min_u, u);
        max_u = max_float(max_u, u);
        min_v = min_float(min_v, v);
        max_v = max_float(max_v, v);
    }

    float u_range = max_u - min_u;
    float v_range = max_v - min_v;

    if (u_range < 1e-6f) u_range = 1.0f;
    if (v_range < 1e-6f) v_range = 1.0f;
    
    float scale = 1.0f /  max_float(u_range, v_range); // Maintain aspect ratio? 
    // Usually we scale both dims to fit 0-1 independently OR uniformly.
    // LSCM preserves angles, so we MUST scale uniformly.
    // "Scale to fit [0,1]²" usually implies uniform scaling to fit bounding box.
    // Re-reading packing: "Scale all UVs by 1.0 / max(max_width, max_height)"
    // Here we just normalize to unit square. Let's do uniform scaling.
    
    for (int i = 0; i < num_verts; i++) {
        uvs[i * 2] = (uvs[i * 2] - min_u) * scale;
        uvs[i * 2 + 1] = (uvs[i * 2 + 1] - min_v) * scale;
    }
}

float* lscm_parameterize(const Mesh* mesh,
                         const int* face_indices,
                         int num_faces) {
    if (!mesh || !face_indices || num_faces == 0) return NULL;

    printf("LSCM parameterizing %d faces...\n", num_faces);

    // STEP 1: Local vertex mapping
    std::map<int, int> global_to_local;
    std::vector<int> local_to_global;
    
    for (int i = 0; i < num_faces; i++) {
        int f = face_indices[i];
        for (int j = 0; j < 3; j++) {
            int v_global = mesh->triangles[f*3 + j];
            if (global_to_local.find(v_global) == global_to_local.end()) {
                global_to_local[v_global] = local_to_global.size();
                local_to_global.push_back(v_global);
            }
        }
    }

    int n = local_to_global.size();
    printf("  Island has %d vertices\n", n);

    if (n < 3) {
        fprintf(stderr, "LSCM: Island too small (%d vertices)\n", n);
        return NULL;
    }

    // STEP 2: Build sparse matrix
    // We are solving Ax = 0 roughly (energy minimization).
    // The energy is sum of squared conformal violation.
    // Referencing LSCM paper / standard implementation:
    // Variables are (u, v).
    // It's often formulated as a complex number problem or 2n x 2n system.
    // We'll use the 2n implementation.
    // Reference: "Least Squares Conformal Maps for Automatic Texture Atlas Generation", Levy et al.
    
    typedef Eigen::Triplet<double> T;
    std::vector<T> triplets;
    
    // Reserve estimation
    triplets.reserve(num_faces * 9 * 4); // rough guess relative to faces

    // For each triangle, project to local 2D and add terms
    for (int i = 0; i < num_faces; i++) {
        int f = face_indices[i];
        int v0_g = mesh->triangles[f*3 + 0];
        int v1_g = mesh->triangles[f*3 + 1];
        int v2_g = mesh->triangles[f*3 + 2];
        
        int idx[3] = {global_to_local[v0_g], global_to_local[v1_g], global_to_local[v2_g]};
        
        Vec3 p0 = get_vertex_position(mesh, v0_g);
        Vec3 p1 = get_vertex_position(mesh, v1_g);
        Vec3 p2 = get_vertex_position(mesh, v2_g);
        
        // Project to local 2D
        // Origin at p0
        // X axis along p1-p0
        Vec3 x_axis = vec3_normalize(vec3_sub(p1, p0));
        Vec3 z_axis = vec3_normalize(vec3_cross(vec3_sub(p1, p0), vec3_sub(p2, p0)));
        Vec3 y_axis = vec3_cross(z_axis, x_axis);
        
        Vec2 l0 = {0, 0};
        Vec2 l1 = {vec3_length(vec3_sub(p1, p0)), 0};
        Vec2 l2 = {vec3_dot(vec3_sub(p2, p0), x_axis), vec3_dot(vec3_sub(p2, p0), y_axis)};
        
        double area = 0.5 * (l1.x * l2.y - l1.y * l2.x); // Should be > 0 if winding is correct
        if (abs(area) < 1e-8) continue;
        
        // W = i * area / Sqrt(area) ? No.
        // The standard LSCM coeff for triangle (0,1,2):
        // M = 1/sqrt(Area) * ...
        // We use the gradient formulation.
        // Grad u = [du/dx, du/dy]
        // Conformality: du/dx = dv/dy, du/dy = -dv/dx
        // Min int ||grad u - (-grad v_perp)||^2
        
        // Using formula from Levy et al (simplified):
        // W_j = (x, y) coordinates
        // ... I'll follow standard coefficients based on cotangents or local coords.
        // Let's use local coords (x,y) of the triangle.
        
    // double s = 1.0 / sqrt(2.0 * area); // Normalization factor (unused in gradient formulation)

        
        // Coefficients for each vertex in the linear combination for gradients
        // For vertex k in triangle, the complex coeff is ???
        // Let's rely on the explicit matrix assembly form.
        // M * [u0 v0 ... un vn]^T = 0
        // M has 2 rows per triangle approx? No, M is 2n x 2n.
        // Wait, LSCM usually minimizes E = X^T A X. So A is symmetric?
        // Or E = || M U ||^2 -> U^T M^T M U = 0
        
        // Implementation approach:
        // We write the conformality condition for each triangle as a linear eq.
        // For triangle T, we want (u1-u0) + i(v1-v0) = Z * ((u2-u0)+i(v2-v0))
        // where Z is complex ratio relating p0, p1, p2.
        
        // Alternative: Use local gradients.
        // For triangle with local coords (x0,y0), (x1,y1), (x2,y2).
        // W_0 = (x2-x1) + i(y2-y1) / 2A
        // W_1 = (x0-x2) + i(y0-y2) / 2A
        // W_2 = (x1-x0) + i(y1-y0) / 2A
        // Condition: Sum W_j * (u_j + i v_j) = 0
        // Real part: Sum (Re(W_j)u_j - Im(W_j)v_j) = 0
        // Imag part: Sum (Im(W_j)u_j + Re(W_j)v_j) = 0
        
        // Correct.
        // double x[] = {l0.x, l1.x, l2.x};
        // double y[] = {l0.y, l1.y, l2.y};
        
        // Re(W_0) = (x[2]-x[1])/(2*area); Im(W_0) = (y[2]-y[1])/(2*area);
        // ...
        
        // But we want to construct the matrix A where we solve A x = b (with locks).
        
        // Matrix M where M * U = 0. Size 2F x 2V?
        // Energy is ||M U||^2. We minimize U^T M^T M U.
        // The system to solve is M^T M U = 0 (subject to constraints).
        // Let's build M directly.
        // Rows: 2 per triangle (Real and Imag equations).
        // Cols: 2 per vertex (u and v indices).
        
        double x[] = {l0.x, l1.x, l2.x};
        double y[] = {l0.y, l1.y, l2.y};
        
        for (int k = 0; k < 3; k++) {
            int prev = (k+2)%3;
            int next = (k+1)%3;
            
            double re_W = (x[prev] - x[next]) / (2.0 * area); // Re(W_k)
            double im_W = (y[prev] - y[next]) / (2.0 * area); // Im(W_k)
            
            // Equation 1 (Real): Sum Re(W)*u - Im(W)*v = 0
            // Row 2*i. Cols: u_idx, v_idx.
            // Coeffs: re_W for u_k, -im_W for v_k
            
            // Equation 2 (Imag): Sum Im(W)*u + Re(W)*v = 0
            // Row 2*i+1. Cols: u_idx, v_idx.
            // Coeffs: im_W for u_k, re_W for v_k
            
            int u_idx = idx[k] * 2;
            int v_idx = idx[k] * 2 + 1;
            
            // Weight by sqrt(area) to make energy = integral?
            // Yes, multiply equations by sqrt(area).
            // Then effective coeffs are divided by 2*sqrt(area) instead of 2*area.
            
            double factor = sqrt(area);
            re_W *= factor;
            im_W *= factor;
            
            // Add to M (which is huge, F*2 rows, V*2 cols)
            // But we actually need M^T M.
            // Constructing M^T M directly is hard locally? 
            // Often easier to build M, then M^T * M.
            // But M is very sparse.
            
            // We use row index base = i * 2.
            int row_r = i * 2;
            int row_i = i * 2 + 1;
            
            triplets.push_back(T(row_r, u_idx, re_W));
            triplets.push_back(T(row_r, v_idx, -im_W));
            
            triplets.push_back(T(row_i, u_idx, im_W));
            triplets.push_back(T(row_i, v_idx, re_W));
        }
    }
    
    // Assemble M
    Eigen::SparseMatrix<double> M(2 * num_faces, 2 * n);
    M.setFromTriplets(triplets.begin(), triplets.end());
    
    // Compute A = M^T * M
    Eigen::SparseMatrix<double> A = M.transpose() * M;
    
    // STEP 3: Boundary conditions
    // Fix two vertices to resolve translation/rotation/scale ambiguity (conformal map).
    // Find vertices to pin.
    int pinned_idx1 = 0;
    int pinned_idx2 = n - 1; // Default
    
    int* boundary_verts = NULL;
    int num_boundary = find_boundary_vertices(mesh, face_indices, num_faces, &boundary_verts);
    if (num_boundary >= 2) {
        // Find two farthest boundary vertices
        float max_dist = -1.0f;
        for (int i = 0; i < num_boundary; i++) {
            for (int k = i + 1; k < num_boundary; k++) {
                // Approximate distance using local array index diff or just pick first/mid
                // Using 3D distance
                Vec3 p1 = get_vertex_position(mesh, local_to_global[global_to_local[boundary_verts[i]]]);
                Vec3 p2 = get_vertex_position(mesh, local_to_global[global_to_local[boundary_verts[k]]]);
                float d = vec3_length(vec3_sub(p1, p2));
                if (d > max_dist) {
                    max_dist = d;
                    pinned_idx1 = global_to_local[boundary_verts[i]];
                    pinned_idx2 = global_to_local[boundary_verts[k]];
                }
            }
        }
        // Optimize: brute force on boundary is O(B^2), OK for small boundaries.
        // Cap B?
        if (num_boundary > 200) {
           pinned_idx1 = global_to_local[boundary_verts[0]];
           pinned_idx2 = global_to_local[boundary_verts[num_boundary/2]];
        }
        free(boundary_verts);
    } else {
        // Closed mesh or weird case. Just pick 0 and largest distance from 0.
        float max_dist = -1;
        Vec3 p0 = get_vertex_position(mesh, local_to_global[pinned_idx1]);
        for(int i=1; i<n; i++) {
            Vec3 p = get_vertex_position(mesh, local_to_global[i]);
            float d = vec3_length(vec3_sub(p0, p));
            if(d > max_dist) {
                max_dist = d;
                pinned_idx2 = i;
            }
        }
    }
    
    // Penalty method or eliminate rows/cols?
    // Eliminate is better for condition number.
    // Replace rows/cols of pinned variables with identity.
    // Pinned: u1=0, v1=0, u2=1, v2=0? (Fix scale/rot)
    
    int p1_u = pinned_idx1 * 2;
    int p1_v = pinned_idx1 * 2 + 1;
    int p2_u = pinned_idx2 * 2;
    int p2_v = pinned_idx2 * 2 + 1;
    
    std::vector<int> pinned_vars = {p1_u, p1_v, p2_u, p2_v};
    double pinned_vals[] = {0.0, 0.0, 1.0, 0.0}; // Pin to (0,0) and (1,0)
    
    // Modify A and b for locking
    // For each pinned var `j` with value `val`:
    //   A(j, :) = 0, A(:, j) = 0, A(j, j) = 1
    //   b = b - column(j) * val  (remove influence from RHS)
    //   b(j) = val
    
    // Since b is initially 0, we only subtract columns.
    Eigen::VectorXd b = Eigen::VectorXd::Zero(2*n);
    
    // This modification on SparseMatrix is slow if done element-wise.
    // Better to use "set to zero" via iterator or reconstruction?
    // Eigen has specific prune/modify methods but it's tricky.
    // A common trick: add large value to diagonal? Soft constraint.
    // Or set row/col to 0 and diag to 1.
    // iterating strictly to zero out is hard.
    
    // Let's use the Large Diagonal approach (Penalty) for simplicity and speed?
    // No, Part 1 requires accuracy.
    // Let's iterate over triplets of A? A is already built.
    
    // Re-build A with constraints? No.
    // Modify A: prune is expensive.
    
    // Actually, usually we remove the variables from the system (Matrix slicing).
    // But keeping 2n sizes is easier for indexing.
    // Iterate over A to zero out rows/cols?
    // Let's use soft constraints with very high weight? 
    // Weight = 1e8 * max(diag).
    // This is "Penalty Method".
    
    // double penalty = 1e8; 
    
    // Better: Algebraic modification
    // For each pinned index `p`:
    // b += A.col(p) * -target_val ? No, we want u_p = target.
    // The equation was A*x = 0.
    // Now x_p is fixed.
    // A_{row} * x + A_{row,p} * x_p = 0
    // A_{row} * x = -A_{row,p} * x_p
    // So b_{row} -= A_{row,p} * x_p.
    // Then set row p to (0...1...0) and b_p = target.
    
    // We can do this efficiently:
    for (int k = 0; k < 4; k++) {
         int idx = pinned_vars[k];
         double val = pinned_vals[k];
         
         // Move column to RHS
         // Iterate over column is hard in RowMajor, but A is ColMajor by default.
         // A is symmetric, so Col == Row.
         
         for (Eigen::SparseMatrix<double>::InnerIterator it(A, idx); it; ++it) {
             if (it.row() != idx) {
                 b(it.row()) -= it.value() * val;
             }
         }
    }
    
    // Now set rows/cols to identity
    // This is modifying the matrix structure, potentially.
    // But setting to 0 doesn't change sparsity if we don't prune.
    // Just setting values to 0.
    
    for (int k = 0; k < 4; k++) {
        int idx = pinned_vars[k];
        
        // Zero out row and col
        // Since symmetric, just iterate once?
        // But need to ensure we visit all entries.
        // It's easier to iterate the whole matrix or use setIdentity?
        // Pruning is best.
    }
    
    // OK, efficiently:
    // 1. Build triplets for A directly instead of M^T * M? Hard.
    // 2. Prune A.
    // Let's iterate A and keep only non-pinned entries, or diagonal for pinned.
    
    std::vector<T> A_triplets;
    // Extract triplets from A
    for (int k = 0; k < A.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(A, k); it; ++it) {
             int r = it.row();
             int c = it.col();
             
             bool r_pin = false, c_pin = false;
             for(int p : pinned_vars) if(p == r) r_pin = true;
             for(int p : pinned_vars) if(p == c) c_pin = true;
             
             if (r_pin || c_pin) {
                 if (r == c) A_triplets.push_back(T(r, c, 1.0));
             } else {
                 A_triplets.push_back(T(r, c, it.value()));
             }
        }
    }
    
    // Set RHS for pinned
    for (int k = 0; k < 4; k++) {
        b(pinned_vars[k]) = pinned_vals[k];
    }
    
    Eigen::SparseMatrix<double> A_final(2*n, 2*n);
    A_final.setFromTriplets(A_triplets.begin(), A_triplets.end());
    
    // STEP 4: Solve
    Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
    solver.compute(A_final);
    
    if (solver.info() != Eigen::Success) {
        fprintf(stderr, "LSCM: Decomposition failed\n");
        return NULL;
    }
    
    Eigen::VectorXd x = solver.solve(b);
    
    if (solver.info() != Eigen::Success) {
        fprintf(stderr, "LSCM: Solve failed\n");
        return NULL;
    }

    // STEP 5: Extract UVs
    float* uvs = (float*)malloc(n * 2 * sizeof(float));
    for (int i = 0; i < n; i++) {
        uvs[i * 2 + 0] = (float)x[i * 2 + 0];
        uvs[i * 2 + 1] = (float)x[i * 2 + 1];
    }

    normalize_uvs_to_unit_square(uvs, n);

    printf("  LSCM completed\n");
    return uvs;
}
