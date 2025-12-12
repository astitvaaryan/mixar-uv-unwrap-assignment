"""
Multi-threaded batch processor - Simplified implementation
"""

from concurrent.futures import ThreadPoolExecutor, as_completed
import threading
import time
import os
from pathlib import Path
from . import bindings, metrics


class UnwrapProcessor:
    """Multi-threaded UV unwrapping batch processor"""

    def __init__(self, num_threads=None):
        self.num_threads = num_threads or os.cpu_count()
        self.progress_lock = threading.Lock()
        self.completed = 0

    def process_batch(self, input_files, output_dir, params, on_progress=None):
        """Process multiple meshes in parallel"""
        os.makedirs(output_dir, exist_ok=True)
        
        results = []
        total = len(input_files)
        self.completed = 0
        start_time = time.time()
        
        with ThreadPoolExecutor(max_workers=self.num_threads) as executor:
            # Submit all tasks
            futures = {
                executor.submit(self._process_single, f, output_dir, params): f
                for f in input_files
            }
            
            # Collect results as they complete
            for future in as_completed(futures):
                filename = futures[future]
                try:
                    result = future.result()
                    results.append(result)
                except Exception as e:
                    results.append({'file': filename, 'error': str(e)})
                
                # Update progress (thread-safe)
                with self.progress_lock:
                    self.completed += 1
                    if on_progress:
                        on_progress(self.completed, total, Path(filename).name)
        
        total_time = time.time() - start_time
        summary = self._compute_summary(results, total_time)
        
        return {
            'summary': summary,
            'files': results
        }

    def _process_single(self, input_path, output_dir, params):
        """Process single file"""
        start_time = time.time()
        
        # Load mesh
        mesh = bindings.load_mesh(input_path)
        
        # Unwrap
        unwrapped, result_metrics = bindings.unwrap(mesh, params)
        
        # Compute detailed metrics
        stretch = metrics.compute_stretch(unwrapped, unwrapped.uvs)
        coverage = metrics.compute_coverage(unwrapped.uvs, unwrapped.triangles)
        
        # Save output
        output_path = Path(output_dir) / Path(input_path).name
        bindings.save_mesh(unwrapped, str(output_path))
        
        elapsed = time.time() - start_time
        
        return {
            'file': str(input_path),
            'vertices': mesh.num_vertices,
            'triangles': mesh.num_triangles,
            'time': elapsed,
            'metrics': {
                'num_islands': result_metrics['num_islands'],
                'stretch': stretch,
                'coverage': coverage,
                'avg_stretch': result_metrics['avg_stretch'],
                'max_stretch': result_metrics['max_stretch'],
            }
        }

    def _compute_summary(self, results, total_time):
        """Compute summary statistics"""
        success = sum(1 for r in results if 'error' not in r)
        failed = len(results) - success
        
        if success > 0:
            avg_time = total_time / len(results)
            avg_stretch = sum(r['metrics']['stretch'] for r in results if 'error' not in r) / success
            avg_coverage = sum(r['metrics']['coverage'] for r in results if 'error' not in r) / success
        else:
            avg_time = avg_stretch = avg_coverage = 0.0
        
        return {
            'total': len(results),
            'success': success,
            'failed': failed,
            'total_time': total_time,
            'avg_time': avg_time,
            'avg_stretch': avg_stretch,
            'avg_coverage': avg_coverage,
        }
