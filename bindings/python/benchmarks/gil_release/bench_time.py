#!/usr/bin/env python
"""Time the workloads while another Python thread competes for the GIL.

Run with the build's bin directory as working directory and on PYTHONPATH, e.g.
    cd build/<preset>/bin
    PYTHONPATH=$PWD python <this dir>/bench_time.py --label release_all_but_scalars --out release_all_but_scalars.json
"""
import argparse
import json
import statistics
import threading
import time

import workloads

LOADS = ['none', 'periodic', 'busy', 'opendaq']


class Background:
    """A second Python thread running during the measurement; counts its iterations."""

    def __init__(self, kind):
        self.kind = kind
        self.stop = threading.Event()
        self.count = 0
        self.thread = None

    def run(self):
        if self.kind == 'busy':
            # Pure Python work all the time, e.g. a computation
            n = 0
            while not self.stop.is_set():
                for _ in range(1000):
                    n += 1
                self.count += 1
        elif self.kind == 'periodic':
            # Timer-like: wake up every 1 ms and do ~100 us of Python work
            while not self.stop.is_set():
                time.sleep(0.001)
                end = time.perf_counter() + 100e-6
                while time.perf_counter() < end:
                    pass
                self.count += 1
        elif self.kind == 'opendaq':
            # Another thread also calling openDAQ
            while not self.stop.is_set():
                workloads.property_read()
                self.count += 1

    def __enter__(self):
        if self.kind != 'none':
            self.thread = threading.Thread(target=self.run, daemon=True)
            self.thread.start()
            time.sleep(0.05)
        self.start = time.perf_counter()
        return self

    def __exit__(self, *exc):
        self.elapsed = time.perf_counter() - self.start
        self.stop.set()
        if self.thread:
            self.thread.join()

    def rate(self):
        return self.count / self.elapsed if self.thread else None


def measure(fn, budget):
    """Returns (median, min) time per op over batches, and the number of ops run in about budget seconds."""
    # Calibrate the batch size so one batch takes about a tenth of the budget
    n = 1
    while True:
        start = time.perf_counter()
        for _ in range(n):
            fn()
        if time.perf_counter() - start > budget / 10 or n >= 1 << 20:
            break
        n *= 4

    per_op = []
    deadline = time.perf_counter() + budget
    while time.perf_counter() < deadline or len(per_op) < 5:
        start = time.perf_counter()
        for _ in range(n):
            fn()
        per_op.append((time.perf_counter() - start) / n)
    return statistics.median(per_op), min(per_op), len(per_op) * n


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--label', required=True, help='variant name stored with the results')
    parser.add_argument('--out', required=True, help='JSON file to write')
    parser.add_argument('--budget', type=float, default=2.0, help='seconds per workload and load')
    parser.add_argument('--loads', nargs='*', choices=LOADS, default=LOADS)
    parser.add_argument('--workloads', nargs='*', help='workload names (default: all)')
    args = parser.parse_args()

    workloads.setup()
    results = []
    for load in args.loads:
        for fn in workloads.select(args.workloads):
            fn()  # warm up
            with Background(load) as bg:
                median, best, ops = measure(fn, args.budget)
            rate = bg.rate()
            results.append(dict(variant=args.label, load=load, workload=fn.__name__, median_us=median * 1e6,
                                min_us=best * 1e6, ops=ops, bg_iter_per_s=rate))
            print(f'{args.label:6} {load:9} {fn.__name__:22} median {median * 1e6:11.2f} us  min {best * 1e6:11.2f} us'
                  f'  ops {ops:8}' + (f'  bg {rate:10.0f}/s' if rate is not None else ''), flush=True)

    with open(args.out, 'w') as f:
        json.dump(results, f, indent=1)
    workloads.teardown()


if __name__ == '__main__':
    main()
