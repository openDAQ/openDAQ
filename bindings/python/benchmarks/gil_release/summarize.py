#!/usr/bin/env python
"""Print markdown tables from the JSON files written by bench_time.py and count_releases.py.

Usage: summarize.py <results dir> [--baseline LABEL]

Times are the median over rounds of each round's median time per op.
"""
import argparse
import glob
import json
import os
import statistics
from collections import defaultdict


def fmt_time(us):
    return f'{us:.2f} us' if us < 1000 else f'{us / 1000:.2f} ms'


def ordered(values):
    return list(dict.fromkeys(values))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('results')
    parser.add_argument('--baseline', help='variant to compare against (default: the first one found)')
    args = parser.parse_args()

    times = defaultdict(list)
    bg_rates = defaultdict(list)
    releases = {}
    variants, loads, names = [], [], []
    for path in sorted(glob.glob(os.path.join(args.results, '*.json'))):
        for r in json.load(open(path)):
            variants.append(r['variant'])
            names.append(r['workload'])
            if 'releases_per_op' in r:
                releases[(r['workload'], r['variant'])] = r['releases_per_op']
                continue
            loads.append(r['load'])
            times[(r['load'], r['workload'], r['variant'])].append(r['median_us'])
            if r['bg_iter_per_s'] is not None:
                bg_rates[(r['load'], r['workload'], r['variant'])].append(r['bg_iter_per_s'])
    variants, loads, names = ordered(variants), ordered(loads), ordered(names)
    baseline = args.baseline or variants[0]
    others = [v for v in variants if v != baseline]
    variants = [baseline] + others

    if releases:
        print('\n### GIL releases per op (main thread)\n')
        print('| Workload | ' + ' | '.join(variants) + ' |')
        print('|---|' + '---:|' * len(variants))
        for name in names:
            print(f'| {name} | ' + ' | '.join(f'{releases.get((name, v), float("nan")):g}' for v in variants) + ' |')

    for load in loads:
        print(f'\n### Time per op, background load: {load}\n')
        header = f'| Workload | {baseline} |' + ''.join(f' {v} | {v}/{baseline} |' for v in others)
        if load != 'none':
            header += ' background iterations/s (' + ' / '.join(variants) + ') |'
        print(header)
        print('|---|---:|' + '---:|---:|' * len(others) + ('---:|' if load != 'none' else ''))
        for name in names:
            median = {v: statistics.median(times[(load, name, v)]) for v in variants if times[(load, name, v)]}
            if baseline not in median:
                continue
            row = f'| {name} | {fmt_time(median[baseline])} |'
            for v in others:
                row += f' {fmt_time(median[v])} | {median[v] / median[baseline]:.2f}x |'
            if load != 'none':
                row += ' ' + ' / '.join(f'{statistics.median(bg_rates[(load, name, v)]):.0f}' for v in variants) + ' |'
            print(row)


if __name__ == '__main__':
    main()
