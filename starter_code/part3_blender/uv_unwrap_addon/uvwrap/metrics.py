"""
Quality metrics for UV mappings - Simplified implementation
"""

import numpy as np


def compute_stretch(mesh, uvs):
    """Compute maximum stretch across all triangles"""
    max_stretch = 1.0
    
    for tri_idx in range(mesh.num_triangles):
        # Get triangle vertices
        v0_idx, v1_idx, v2_idx = mesh.triangles[tri_idx]
        
        # 3D positions
        p0 = mesh.vertices[v0_idx]
        p1 = mesh.vertices[v1_idx]
        p2 = mesh.vertices[v2_idx]
        
        # UV positions
        uv0 = uvs[v0_idx]
        uv1 = uvs[v1_idx]
        uv2 = uvs[v2_idx]
        
        # Edge vectors
        dp1 = p1 - p0
        dp2 = p2 - p0
        duv1 = uv1 - uv0
        duv2 = uv2 - uv0
        
        # Build Jacobian matrix
        try:
            # UV->3D mapping: J = [dp1, dp2] @ inv([[duv1], [duv2]])
            uv_mat = np.column_stack([duv1, duv2])
            if np.linalg.det(uv_mat) == 0:
                continue
            
            pos_mat = np.column_stack([dp1, dp2])
            J = pos_mat @ np.linalg.inv(uv_mat)
            
            # Compute SVD
            _, s, _ = np.linalg.svd(J, full_matrices=False)
            if len(s) >= 2 and s[1] > 1e-8:
                stretch = max(s[0] / s[1], s[1] / s[0])
                max_stretch = max(max_stretch, stretch)
        except:
            pass
    
    return float(max_stretch)


def compute_coverage(uvs, triangles, resolution=256):
    """Compute UV coverage percentage"""
    grid = np.zeros((resolution, resolution), dtype=bool)
    
    for tri in triangles:
        uv0 = np.clip(uvs[tri[0]] * resolution, 0, resolution-1).astype(int)
        uv1 = np.clip(uvs[tri[1]] * resolution, 0, resolution-1).astype(int)
        uv2 = np.clip(uvs[tri[2]] * resolution, 0, resolution-1).astype(int)
        
        # Simple bounding box rasterization
        min_x = max(0, min(uv0[0], uv1[0], uv2[0]))
        max_x = min(resolution-1, max(uv0[0], uv1[0], uv2[0]))
        min_y = max(0, min(uv0[1], uv1[1], uv2[1]))
        max_y = min(resolution-1, max(uv0[1], uv1[1], uv2[1]))
        
        # Mark pixels in bounding box
        grid[min_y:max_y+1, min_x:max_x+1] = True
    
    return float(np.sum(grid) / (resolution * resolution))


def compute_angle_distortion(mesh, uvs):
    """Compute maximum angle distortion"""
    max_distortion = 0.0
    
    for tri_idx in range(mesh.num_triangles):
        v0_idx, v1_idx, v2_idx = mesh.triangles[tri_idx]
        
        # 3D triangle
        p0, p1, p2 = mesh.vertices[v0_idx], mesh.vertices[v1_idx], mesh.vertices[v2_idx]
        e0_3d = p1 - p0
        e1_3d = p2 - p0
        
        # UV triangle
        uv0, uv1, uv2 = uvs[v0_idx], uvs[v1_idx], uvs[v2_idx]
        e0_2d = uv1 - uv0
        e1_2d = uv2 - uv0
        
        # Compute angle at vertex 0
        try:
            angle_3d = np.arccos(np.clip(np.dot(e0_3d, e1_3d) / (np.linalg.norm(e0_3d) * np.linalg.norm(e1_3d)), -1, 1))
            angle_2d = np.arccos(np.clip(np.dot(e0_2d, e1_2d) / (np.linalg.norm(e0_2d) * np.linalg.norm(e1_2d)), -1, 1))
            distortion = abs(angle_3d - angle_2d)
            max_distortion = max(max_distortion, distortion)
        except:
            pass
    
    return float(max_distortion)
