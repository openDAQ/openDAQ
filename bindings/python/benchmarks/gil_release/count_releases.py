#!/usr/bin/env python
"""Count how often each workload releases the GIL on the main thread.

Every release can hand the GIL to another Python thread and cost up to a switch
interval (5 ms by default) to get it back, so the count is a deterministic proxy
for the cost under contention. Counts are taken with gdb: a breakpoint on
PyEval_SaveThread (called by py::gil_scoped_release and
py::release_gil_before_calling_cpp_dtor), and getppid() calls as markers around
each workload.

Run with the build's bin directory as working directory and on PYTHONPATH, e.g.
    cd build/<preset>/bin
    PYTHONPATH=$PWD python <this dir>/count_releases.py --label release_all_but_scalars --out release_all_but_scalars_releases.json
Requires gdb. Re-launches itself under gdb.
"""
import argparse
import json
import os
import subprocess
import sys

OPS = 50
MARKER = 'GIL_RELEASES'

GDB_SCRIPT = f'''
import gdb

releases = 0
marks = []

class CountReleases(gdb.Breakpoint):
    def stop(self):
        global releases
        if gdb.selected_thread().num == 1:
            releases += 1
        return False

class Mark(gdb.Breakpoint):
    def stop(self):
        if gdb.selected_thread().num == 1:
            marks.append(releases)
            if len(marks) % 2 == 0:
                gdb.write(f'{MARKER} {{marks[-1] - marks[-2]}}\\n')
        return False

gdb.execute('set breakpoint pending on')
gdb.execute('set pagination off')
CountReleases('PyEval_SaveThread', internal=True)
Mark('getppid', internal=True)
gdb.execute('run')
'''


def run_workloads(names):
    """Inferior side: run each workload OPS times between two markers."""
    import workloads
    workloads.setup()
    for fn in workloads.select(names):
        for _ in range(5):
            fn()  # warm up, so one-time initialization isn't counted
        os.getppid()
        for _ in range(OPS):
            fn()
        os.getppid()
        print(f'WORKLOAD {fn.__name__}', flush=True)
    workloads.teardown()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--label', required=True, help='variant name stored with the results')
    parser.add_argument('--out', required=True, help='JSON file to write')
    parser.add_argument('--workloads', nargs='*', help='workload names (default: all)')
    parser.add_argument('--inferior', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()

    if args.inferior:
        run_workloads(args.workloads)
        return

    script = os.path.join(os.path.dirname(os.path.abspath(args.out)), f'.count_releases_{os.getpid()}.py')
    with open(script, 'w') as f:
        f.write(GDB_SCRIPT)
    try:
        cmd = ['gdb', '-q', '-batch', '-x', script, '--args', sys.executable, os.path.abspath(__file__), '--inferior',
               '--label', args.label, '--out', args.out]
        if args.workloads:
            cmd += ['--workloads', *args.workloads]
        output = subprocess.run(cmd, capture_output=True, text=True, check=True).stdout
    finally:
        os.remove(script)

    counts = [int(line.split()[1]) for line in output.splitlines() if line.startswith(MARKER)]
    names = [line.split()[1] for line in output.splitlines() if line.startswith('WORKLOAD ')]
    if not names or len(counts) != len(names):
        sys.exit(f'Unexpected gdb output:\n{output}')

    results = [dict(variant=args.label, workload=name, releases_per_op=count / OPS) for name, count in zip(names, counts)]
    for r in results:
        print(f'{args.label:6} {r["workload"]:22} {r["releases_per_op"]:7.2f} GIL releases per op')
    with open(args.out, 'w') as f:
        json.dump(results, f, indent=1)


if __name__ == '__main__':
    main()
