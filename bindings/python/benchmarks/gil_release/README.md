# GIL release on wrapper destruction

Benchmarks for the cost of releasing the GIL before destroying wrapped openDAQ objects
(`py::release_gil_before_calling_cpp_dtor`), measured on typical Python workloads rather than a
single dealloc.

## Contents

| File | Purpose |
|---|---|
| `workloads.py` | The workloads and their fixtures (instance with a reference device, property object, stream reader) |
| `bench_time.py` | Times each workload while a second Python thread competes for the GIL |
| `count_releases.py` | Counts GIL releases per op on the main thread, using gdb (deterministic) |
| `run_variants.sh` | Runs both for several builds of the module, interleaved, and prints the summary |
| `summarize.py` | Turns the JSON results into markdown tables |

### Workloads

| Workload | One op |
|---|---|
| `scalars` | Construct and drop 10 each of `Integer`, `Float`, `String`, `Boolean` |
| `list_build` | Build and drop a `List` of 20 integers |
| `property_read` | 20 `get_property_value` calls (values convert to native Python types) |
| `property_iterate` | Iterate 20 `visible_properties`, read `name`, `value_type`, `default_value` |
| `property_object_create` | Create a `PropertyObject` with 5 `IntProperty`s and drop it |
| `device_tree_walk` | For every signal of a reference device: `name`, `global_id`, `descriptor` with `sample_type`, `unit`, `rule` |
| `channel_props` | For every channel: read every visible property's value |
| `stream_read` | `StreamReader.read(100, 0)` |
| `packet_create` | Build a descriptor and a `DataPacket` and drop them |

### Background loads

| Load | Second Python thread |
|---|---|
| `none` | none |
| `periodic` | timer-like: sleeps 1 ms, then does ~100 us of Python work |
| `busy` | pure Python work all the time |
| `opendaq` | runs `property_read` in a loop |

## Running

The variants differ only in the bindings, so build them in one build directory and keep a copy of
each module:

```bash
cmake --preset x64/clang/full/release -DOPENDAQ_GENERATE_PYTHON_BINDINGS=ON
cmake --build build/x64/clang/full/release        # core, device modules and the Python module
cp build/x64/clang/full/release/bin/opendaq.cpython-*.so /tmp/variants/release_all_but_scalars.so
# edit the bindings (see "Variants"), rebuild only the module and copy it again
ninja -C build/x64/clang/full/release bin/opendaq.cpython-314-x86_64-linux-gnu.so
```

Make sure the device modules in `bin` were built from the same SDK as the core libraries,
otherwise `add_device('daqref://device0')` fails.

```bash
PYTHON=.venv/bin/python bindings/python/benchmarks/gil_release/run_variants.sh \
    build/x64/clang/full/release/bin /tmp/gil-results 3 \
    release_none=/tmp/variants/release_none.so \
    release_all=/tmp/variants/release_all.so \
    release_all_but_scalars=/tmp/variants/release_all_but_scalars.so
```

The first variant is the baseline. `BUDGET` sets the seconds per measurement (default 2); three
rounds take about 15 minutes. The module in `bin` is restored afterwards. The scripts can also be
run on their own; see their `--help`. `count_releases.py` needs gdb.

## Results

Run on 2026-10-22, i7-13700HX, Linux 7.0, clang 21.1.8, `x64/clang/full/release`, CPython 3.14.4
(GIL enabled, default 5 ms switch interval), pybind11 3.0.1, 3 interleaved rounds of 2 s per cell.

### Variants

All three are built from the same source and differ only in which classes get
`py::release_gil_before_calling_cpp_dtor`.

| Variant | GIL released before destroying |
|---|---|
| `release_none` | nothing (before "Release GIL before destroying wrapped openDAQ objects") |
| `release_all` | every wrapped interface and `IBaseObject` ("Release GIL before destroying wrapped openDAQ objects") |
| `release_all_but_scalars` | every wrapped interface except scalar values ("Keep GIL when destroying wrapped scalar values") |

The measured builds predate a rebase of this branch and differ from it in ways the workloads don't
exercise: `PyObjectImpl` deferred its release to the main thread instead of acquiring the GIL, the
bound classes that aren't openDAQ interfaces (`TimeReader`, `MockSignal`, `PyEventQueue`) kept the
GIL, and `INumber` also counted as a scalar value.

### GIL releases per op

Counted on the main thread over 50 ops, without a background thread. These counts include the
releases inside the bound methods (`py::gil_scoped_release`), which are the same in all variants.

| Workload | release_none | release_all | release_all_but_scalars |
|---|---:|---:|---:|
| scalars | 0 | 40 | 0 |
| list_build | 0 | 1 | 1 |
| property_read | 60 | 60 | 60 |
| property_iterate | 81 | 103 | 103 |
| property_object_create | 5 | 26 | 11 |
| device_tree_walk | 13 | 23 | 23 |
| channel_props | 113 | 145 | 145 |
| stream_read | 3 | 3 | 3 |
| packet_create | 2 | 5 | 5 |

### Time per op

Median over rounds of the per-round median; each ratio is to `release_none`.

| Workload | release_none | release_all | ratio | release_all_but_scalars | ratio |
|---|---:|---:|---:|---:|---:|
| **No background thread** | | | | | |
| scalars | 9.90 us | 11.27 us | 1.14x | 10.29 us | 1.04x |
| list_build | 4.22 us | 4.26 us | 1.01x | 4.38 us | 1.04x |
| property_read | 25.33 us | 23.35 us | 0.92x | 24.99 us | 0.99x |
| property_iterate | 36.70 us | 34.10 us | 0.93x | 37.43 us | 1.02x |
| property_object_create | 14.91 us | 14.36 us | 0.96x | 14.78 us | 0.99x |
| device_tree_walk | 12.27 us | 11.52 us | 0.94x | 12.14 us | 0.99x |
| channel_props | 95.36 us | 91.50 us | 0.96x | 93.00 us | 0.98x |
| stream_read | 2.17 us | 2.06 us | 0.95x | 2.09 us | 0.96x |
| packet_create | 3.22 us | 3.31 us | 1.03x | 3.37 us | 1.05x |
| **Periodic thread** | | | | | |
| scalars | 10.34 us | 12.71 us | 1.23x | 10.34 us | 1.00x |
| list_build | 4.39 us | 4.84 us | 1.10x | 5.02 us | 1.14x |
| property_read | 26.66 us | 26.33 us | 0.99x | 27.45 us | 1.03x |
| property_iterate | 39.10 us | 39.51 us | 1.01x | 41.32 us | 1.06x |
| property_object_create | 15.49 us | 15.91 us | 1.03x | 15.89 us | 1.03x |
| device_tree_walk | 13.23 us | 12.84 us | 0.97x | 13.10 us | 0.99x |
| channel_props | 102.39 us | 101.18 us | 0.99x | 105.22 us | 1.03x |
| stream_read | 2.41 us | 2.35 us | 0.98x | 2.33 us | 0.97x |
| packet_create | 3.62 us | 3.65 us | 1.01x | 3.70 us | 1.02x |
| **openDAQ thread** | | | | | |
| scalars | 9.87 us | 54.09 us | 5.48x | 10.21 us | 1.03x |
| list_build | 4.25 us | 10.48 us | 2.46x | 4.97 us | 1.17x |
| property_read | 90.10 us | 145.91 us | 1.62x | 85.08 us | 0.94x |
| property_iterate | 112.21 us | 132.83 us | 1.18x | 125.97 us | 1.12x |
| property_object_create | 16.75 us | 44.20 us | 2.64x | 23.77 us | 1.42x |
| device_tree_walk | 25.93 us | 36.18 us | 1.39x | 27.50 us | 1.06x |
| channel_props | 228.68 us | 270.98 us | 1.18x | 247.20 us | 1.08x |
| stream_read | 2.59 us | 3.05 us | 1.18x | 2.60 us | 1.00x |
| packet_create | 6.55 us | 9.14 us | 1.39x | 8.80 us | 1.34x |
| **Busy thread** (bimodal, see below) | | | | | |
| scalars | 19.51 us | 21.63 ms | 1109x | 19.65 us | 1.01x |
| list_build | 8.56 us | 5.58 us | 0.65x | 5.35 us | 0.63x |
| property_read | 44.64 ms | 66.60 ms | 1.49x | 136.55 ms | 3.06x |
| property_iterate | 66.67 ms | 69.25 ms | 1.04x | 112.12 ms | 1.68x |
| property_object_create | 3.94 ms | 22.51 ms | 5.72x | 23.71 ms | 6.02x |
| device_tree_walk | 18.19 ms | 27.56 ms | 1.51x | 26.90 ms | 1.48x |
| channel_props | 146.75 ms | 238.49 ms | 1.63x | 250.76 ms | 1.71x |
| stream_read | 2.65 ms | 1.28 ms | 0.48x | 3.71 ms | 1.40x |
| packet_create | 4.73 ms | 7.70 ms | 1.63x | 5.38 ms | 1.14x |

With the openDAQ thread, the time the main thread gives up isn't lost: the background thread runs
more often. In `scalars` it completes 487 iterations/s under `release_none` and 13,319/s under
`release_all`.

## Findings

- **Uncontended, the cost is negligible.** One extra release and reacquire costs about 35 ns
  (`scalars`: +1.4 us for 40 releases). Every other workload is within ±5%, which is run-to-run noise.
- **Under contention, the cost is the number of extra handoffs.** Each release lets the other thread
  take the GIL, and getting it back can take up to a switch interval. `release_all` adds about
  25–75% more releases to typical openDAQ calls (`property_iterate` 81 → 103, `device_tree_walk`
  13 → 23, `channel_props` 113 → 145). For code that only creates scalars it adds releases where
  there were none (`scalars` 0 → 40).
- **With a thread that never blocks, timings are bimodal.** Either the main thread gets the GIL back
  at once, or it waits a full 5 ms switch interval for every release. Which regime a batch hits
  varies between rounds, even for identical code: `release_all_but_scalars` and `release_all` run
  the same code for `property_read`, yet differ by 2x. Only the large effects in that table are
  meaningful (`scalars`, `property_object_create`); for the rest, use the release counts.
- **The bound methods themselves release the GIL far more often than destruction does.**
  `property_read` releases 3 times per `get_property_value` call in every variant, and
  `channel_props` releases 113 times per op under `release_none`. Under contention that dominates
  the cost in every variant.
- **The scalar exemption (`release_all_but_scalars`) removes the cost for scalars.** `scalars` and
  `property_object_create` (which creates `Integer`, `String` and `Boolean` arguments) drop back
  towards `release_none`. What remains are wrappers that aren't the last owner of their object
  (Property wrappers while iterating, descriptors, units), which keeping the GIL unless the last
  reference is dropped would avoid.

## Caveats

- `count_releases.py` counts every `PyEval_SaveThread` call on the main thread, including any made
  by Python itself, so the counts are upper bounds. They were stable between runs, except
  `device_tree_walk`, which counted 21 in one run and 23 in another.
- The busy and periodic loads are synthetic. A real second thread that blocks often (I/O, waits)
  behaves like `periodic`; one doing sustained Python work behaves like `busy`.
