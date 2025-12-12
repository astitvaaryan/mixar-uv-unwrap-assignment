"""
Parameter optimizer - Simplified grid search implementation
"""

import itertools
from . import bindings, metrics


class ParameterOptimizer:
    """Optimize unwrapping parameters using grid search"""
    
    def __init__(self, mesh):
        self.mesh = mesh
    
    def optimize(self, param_ranges, metric='stretch', verbose=True):
        """
        Find best parameters using grid search
        
        Args:
            param_ranges: Dict of parameter ranges, e.g.:
                {
                    'angle_threshold': [20, 30, 40],
                    'min_island_faces': [3, 5, 10],
                }
            metric: Metric to optimize ('stretch' or 'coverage')
            verbose: Print progress
        
        Returns:
            best_params: Dictionary of best parameters
            best_value: Best metric value
        """
        # Generate all parameter combinations
        param_names = list(param_ranges.keys())
        param_values = list(param_ranges.values())
        combinations = list(itertools.product(*param_values))
        
        best_value = float('inf') if metric == 'stretch' else 0.0
        best_params = None
        
        for i, combo in enumerate(combinations):
            params = dict(zip(param_names, combo))
            
            try:
                # Unwrap with these parameters
                unwrapped, _ = bindings.unwrap(self.mesh, params)
                
                # Compute metric
                if metric == 'stretch':
                    value = metrics.compute_stretch(unwrapped, unwrapped.uvs)
                    is_better = value < best_value
                elif metric == 'coverage':
                    value = metrics.compute_coverage(unwrapped.uvs, unwrapped.triangles)
                    is_better = value > best_value
                else:
                    raise ValueError(f"Unknown metric: {metric}")
                
                if verbose:
                    print(f"[{i+1}/{len(combinations)}] Params: {params} -> {metric}={value:.4f}")
                
                if is_better:
                    best_value = value
                    best_params = params.copy()
            
            except Exception as e:
                if verbose:
                    print(f"[{i+1}/{len(combinations)}] Failed: {e}")
        
        return best_params, best_value
