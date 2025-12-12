#!/usr/bin/env python3
"""
UV Unwrap CLI Tool - Complete implementation
"""

import argparse
import sys
from pathlib import Path
from uvwrap import bindings, processor, optimizer


def main():
    parser = argparse.ArgumentParser(description='UV Unwrap CLI Tool')
    subparsers = parser.add_subparsers(dest='command', help='Command to run')
    
    # Unwrap single file
    unwrap_parser = subparsers.add_parser('unwrap', help='Unwrap single mesh')
    unwrap_parser.add_argument('input', help='Input OBJ file')
    unwrap_parser.add_argument('output', help='Output OBJ file')
    unwrap_parser.add_argument('--angle', type=float, default=30.0, help='Angle threshold')
    unwrap_parser.add_argument('--min-faces', type=int, default=5, help='Min island faces')
    unwrap_parser.add_argument('--margin', type=float, default=0.02, help='Island margin')
    unwrap_parser.add_argument('--no-pack', action='store_true', help='Disable packing')
    
    # Batch process
    batch_parser = subparsers.add_parser('batch', help='Process multiple meshes')
    batch_parser.add_argument('input_dir', help='Input directory')
    batch_parser.add_argument('output_dir', help='Output directory')
    batch_parser.add_argument('--threads', type=int, help='Number of threads')
    batch_parser.add_argument('--angle', type=float, default=30.0)
    batch_parser.add_argument('--min-faces', type=int, default=5)
    
    # Optimize parameters
    opt_parser = subparsers.add_parser('optimize', help='Optimize parameters')
    opt_parser.add_argument('input', help='Input OBJ file')
    opt_parser.add_argument('--metric', choices=['stretch', 'coverage'], default='stretch')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    try:
        if args.command == 'unwrap':
            # Load mesh
            print(f"Loading {args.input}...")
            mesh = bindings.load_mesh(args.input)
            print(f"  {mesh.num_vertices} vertices, {mesh.num_triangles} triangles")
            
            # Unwrap
            params = {
                'angle_threshold': args.angle,
                'min_island_faces': args.min_faces,
                'pack_islands': not args.no_pack,
                'island_margin': args.margin,
            }
            print("Unwrapping...")
            unwrapped, metrics = bindings.unwrap(mesh, params)
            
            # Save
            print(f"Saving to {args.output}...")
            bindings.save_mesh(unwrapped, args.output)
            
            print("\nResults:")
            print(f"  Islands: {metrics['num_islands']}")
            print(f"  Avg stretch: {metrics['avg_stretch']:.2f}")
            print(f"  Max stretch: {metrics['max_stretch']:.2f}")
            print(f"  Coverage: {metrics['coverage']*100:.1f}%")
            
        elif args.command == 'batch':
            # Find all OBJ files
            input_dir = Path(args.input_dir)
            files = list(input_dir.glob('*.obj'))
            print(f"Found {len(files)} OBJ files in {input_dir}")
            
            # Process batch
            params = {
                'angle_threshold': args.angle,
                'min_island_faces': args.min_faces,
                'pack_islands': True,
                'island_margin': 0.02,
            }
            
            proc = processor.UnwrapProcessor(num_threads=args.threads)
            
            def progress(cur, total, name):
                pct = int(100 * cur / total)
                print(f"\r[{cur}/{total}] {pct}% - {name}", end='', flush=True)
            
            results = proc.process_batch(
                [str(f) for f in files],
                args.output_dir,
                params,
                on_progress=progress
            )
            
            print(f"\n\nBatch complete:")
            print(f"  Total: {results['summary']['total']}")
            print(f"  Success: {results['summary']['success']}")
            print(f"  Failed: {results['summary']['failed']}")
            print(f"  Total time: {results['summary']['total_time']:.1f}s")
            print(f"  Avg time: {results['summary']['avg_time']:.2f}s")
            print(f"  Avg stretch: {results['summary']['avg_stretch']:.2f}")
            print(f"  Avg coverage: {results['summary']['avg_coverage']*100:.1f}%")
            
        elif args.command == 'optimize':
            print(f"Loading {args.input}...")
            mesh = bindings.load_mesh(args.input)
            
            print("Optimizing parameters...")
            opt = optimizer.ParameterOptimizer(mesh)
            
            param_ranges = {
                'angle_threshold': [20.0, 30.0, 40.0],
                'min_island_faces': [3, 5, 10],
            }
            
            best_params, best_value = opt.optimize(param_ranges, metric=args.metric)
            
            print(f"\nBest parameters (optimize {args.metric}):")
            print(f"  {best_params}")
            print(f"  {args.metric} = {best_value:.4f}")
        
        return 0
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
