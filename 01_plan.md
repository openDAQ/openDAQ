# MultiReader2 Plan

Greenfield multi reader in the main repo, branch `refactor/multi-reader-2` (off `main`).
Sources: `core/opendaq/reader/*/multi_reader2*` + `multi_reader_data_manager.*`; tests in `tests/test_multi_reader2.cpp` (impl cpps compiled into the test target — the classes are not DLL-exported).

Build: `"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build/x64/msvc-26/full --target test_reader --config Release`
Run: `build/x64/msvc-26/full/bin/Release/test_reader.exe --gtest_filter="MultiReader2Test.*:MultiReaderDataManagerTest.*"`

## Architecture

### Component layout

- **`IMultiReader2`** — public interface. No builder: `IMultiReader2Params` + `configure()` is the only mutation path; the constructor takes params and calls configure. Error-code returns everywhere (ctor surfaces via `checkErrorInfo`).
- **`MultiReader2Impl`** (facade) — `ImplementationOfWeak<IMultiReader2, IInputPortNotifications>`. Owns ports (internal ports for signal inputs, portBinder-adopted external ports), all forced to `SameThread`. Forwards everything stateful to the manager.
- **`MultiReaderDataManager`** — plain C++ engine, value member of the facade, lives for the reader's lifetime. Owns queues, masks, event staging, sync, and the read path.
- **`IMultiReader2Params`** — get/setInputs (homogeneity validated in setter), MainInput, UnusedInputs, ValueReadType (**mandatory** — getter returns `NOTASSIGNED` until set), MinReadCount (>0), RequireSameRates. Setters store owning copies (never `Borrow` into members — ObjectPtr move propagates the borrowed flag).
- **`IMultiReader2Status`** — immutable value object: `getStatus` (Data | Event), `getDomainDescriptor` (main's), `getDescriptors` (id→value descriptor), `getDividers` (id→int, empty until dividers phase), `getErrors` (id→`MultiReader2InputError`: SyncFailed, DataLoss, Gap, InvalidDescriptor, InvalidDomain, Disconnected).

### Threading model

- **Facade notification paths are lock-free**: an immutable `Wiring` snapshot (`unordered_map<IInputPort*, {index, inputId}>`, queryInterface-normalized keys) swapped via `std::atomic_load/store` on `shared_ptr`. `configure` cuts the wire (null snapshot), mutates under the facade mutex, rewires. In-flight callbacks finish on the old snapshot.
- **Manager producer paths are lock-free**: `State` snapshot (same atomic-shared_ptr pattern, swapped wholesale on reconfigure). Per slot an `alignas(64)` `SlotCell`: SPSC dummy-node queue (producer owns tail, consumer owns head), `dataPacketCount`, descriptor-event cache. Masks: `readyMask` / `usedMask` / `connectedMask`; counters `pendingEvents`; flags `parked` / `active` / `armed`. 64-input limit (asserted; widen masks to arrays later).
- **Consumer calls** (read, commitEvent, getAvailableCount, setUsed/setActive, reconfigure, clear) share one `consumerMutex`. Consumer-side per-slot `SlotView`: staged deque (drained from SPSC), stagedSamples, frontOffset, delivered version, committed descriptors, parsed domain facts, failed flag.
- **One producer thread per slot** is a precondition (SameThread ports). Producer-only plain members live inside SlotCell (descriptor merge copies).

### Ingest and wake protocol (`addPacket`)

- **Descriptor events are cache-only, never queued.** Only `DATA_DESCRIPTOR_CHANGED` and `IMPLICIT_DOMAIN_GAP_DETECTED` are accepted; other event packets are dropped. Change packets carry deltas, so the producer keeps plain per-slot value/domain copies (single-writer) and exchanges a rebuilt **full-state merged packet** into the atomic cache (`lastEventPacket`), then bumps `eventVersion` and `pendingEvents`. A gap only bumps a per-slot `gapCount` (plus `pendingEvents`): the next read reports it as an Event with a `Gap` error, the gapped slot's staged data is dropped (it predates a discontinuity), and the commit resyncs. Ownership transfers only by `exchange` on both sides — no load+addRef UAF.
- **Data packets**: dropped while any used input is disconnected, when the reader is inactive, or the slot unused; otherwise queued. Shared words are touched only on empty→non-empty transitions.
- **Wake election**: `deliverable()` = not parked ∧ all used inputs connected ∧ (pendingEvents>0 ∨ (active ∧ all used slots ready)). One producer wins `armed.exchange(false)`; the facade schedules a coalesced notification pass on the openDAQ scheduler (inline single-shot fallback without one). `armDataAvailable` re-checks and reclaims the lost-wakeup window. The facade re-arms after `read`/`commitEvent` so consumption reopens the wake window.
- **Ready-bit semantics**: a slot's ready bit means *fresh queued data* — the consumer lowers it when it drains the SPSC queue, so staged-but-unconsumable data never re-triggers passes (no spin during sync). The contract for callback consumers is therefore *consume until drained*: reads and commits re-arm, and only new packets wake again. While synchronizing, a flowing input wakes the consumer past the 2s deadline (`syncing` + `syncDeadlineTicks` atomics) so silent peers can time out without a background timer.

### Drop-rule table

| state | data packets | descriptor events |
|---|---|---|
| normal | queued | cached + version + wake |
| unused input | dropped at ingest | cached + version + wake |
| inactive reader | dropped at ingest | cached + version + wake |
| any used input disconnected (Waiting) | dropped at ingest | cached + version, **no wake** |
| parked (event window) | queued (no wake) | cached + version (no wake) |
| failed input (sync) | dropped at consumer drain | cached; delivery clears failed |

### Read path (consumer, all under mutex)

1. **Parked** → re-report the same pending status, count 0.
2. **Drain** SPSC → staged (stagedSamples counts only packets matching the committed value descriptor; failed slots discard).
3. **Deliver events**: per slot, version snapshot → exchange cache out → adopt, parse, update committed descriptors, `parseDomain`, clear `failed`, drop stale staged fronts, recount; `deliveredVersion = pre-exchange snapshot` (duplicates possible, loss impossible). `pendingEvents` reset conservatively (store 0, rescan, re-raise).
4. **Fresh disconnect** of a used input (vs `reportedDisconnectMask`) → discard all data, park Event with `Disconnected` errors (delivered descriptors merged in). Commit snapshots the mask, so a committed disconnect leaves the reader **waiting silently** until reconnect (the reconnect's descriptor event reopens the window — `connectInternal` fires `connected` before enqueuing the descriptor, so the last connect's descriptor is never dropped).
5. **Delivered versions** → park Event (during sync: affected used inputs marked `SyncFailed` — they rejoin after commit since delivery refreshed them).
6. **Sync** (when not waiting and not synced) — see below; failures park.
7. Otherwise (synced) the data read: plan = min staged run over used, non-failed inputs, clamped to the request and gated by `minReadCount` (a non-zero request below the minimum is INVALIDPARAMETER; available below it reads 0). Samples are copied per slot with sample-type conversion to ValueReadType (scalar numeric sources only, validated at sync), `packetOffset` = main-tick timestamp of the first sample, `readWithDomain` fills buffer 0 with Int64 main-tick timestamps. Buffers of unused/failed inputs are never written (consumers zero them and sum everything).

`getAvailableCount`: 0 while parked / events pending / waiting / not synced; else min of stagedSamples over used, non-failed slots.

### Event window

`commitEvent` is the single transaction point: only state where `setUsed`/`setActive` are legal (INVALIDSTATE otherwise); mask writes apply immediately, `setUsed(false)`/`setActive(false)` discard the affected staged data, `setUsed(true)` clears `failed`. Commit clears the pending status, recomputes `reportedDisconnectMask`, and **restarts sync** with a fresh 2s window. Reconfigure XOR commitEvent: reconfigure cancels the window (fresh state, gates closed, new handshake pending). Reconfigure is legal inside every callback (tested).

### Bootstrap

After `configure` releases the facade mutex, every connected port gets `IConnectionInternal::enqueueLastDescriptor()` + an explicit `packetReceived` drain (enqueueLastDescriptor does not notify). Read #1 is therefore always a handshake Event carrying all known descriptors; commit → sync → data. This also re-populates producer merge copies after every reconfigure.

### Synchronization (Phase 4, pinned requirements)

- Domain constraints (local validation, at delivery): integer domain sample type, unit quantity "time" symbol "s", implicit **linear** rule only, positive delta/resolution. Missing domain descriptor → `InvalidDescriptor`; broken constraint → `InvalidDomain`.
- Relational (at sync, vs main): effective sample rate equal to main's (equal delta+resolution fast path, else rational period comparison; ticks scaled onto the main lattice), compatible origin, same tick grid (phase). Violations → `InvalidDomain`, immediate. Rate dividers are out of scope (rate must equal main's).
- Origins (ACCEPTED 2026-08-26, not yet implemented — current code still requires equal origin strings): different origins are allowed as long as the origin *type* matches — all ISO-8601 absolute, or all unassigned/relative. Absolute origins are parsed and the epoch difference is converted into a tick offset during alignment; mixing absolute with relative is `InvalidDomain`.
- Incremental, non-blocking, inside `read`: align every participant to the latest next-timestamp (target monotonically increases); discard below target (whole packets + partial via frontOffset). Staged data assumed gap-free (ranges derived from next + stagedSamples).
- **2s hardcoded timeout** from commit: an input with no data past the deadline → `SyncFailed`. Range distance guard: closest points of the main range and an input's range further apart than 2s worth of ticks → `SyncFailed` immediately.
- **Main always succeeds** — at worst synced to itself; with no data it just waits. Main locally invalid → park with main's error after the deadline.
- Event during sync → that input reported `SyncFailed` in the boundary event; the new descriptor lets it rejoin after commit (failure is non-sticky).
- **Failures park** (FB observes and reacts — e.g. setUsed(false)); after commit, failed inputs sit out (drain discards their data, counts skip them) until a new descriptor/reconfigure/setUsed(true) refreshes them.
- Sync completion records `syncedStart` (Phase 5 read anchor). While syncing, read returns Data with count 0.

## Decisions

| decision | rationale |
|---|---|
| No builder; params object + configure; ctor = configure | one mutation path, reconfigure ≡ construct |
| ValueReadType mandatory in params | `SampleType::Invalid == Undefined == 0`, no safe default |
| Error codes, no exceptions (ctor via checkErrorInfo) | reader is engine code; FB loops must not catch |
| Manager is plain C++, fully owned | no ref-count cycles, no interface overhead on hot paths |
| P1: all queue surgery consumer-side | producers only drop-at-ingest and flip flags |
| P2: commitEvent is the single transaction point | one place where state changes take effect |
| Descriptor events cache-only (merged full state), versioned | correctness never depends on queued event survival; newest-wins merge loses nothing because producer merges deltas |
| deliveredVersion = pre-exchange snapshot | duplicate re-report possible, loss impossible |
| Committed descriptor = last delivered | one update point; stale data filtered by per-packet descriptor identity (pointer fast path, deep equals fallback) |
| Disconnect = synthesized boundary; committed disconnect waits silently | `reportedDisconnectMask` recomputed at commit |
| Wake = single-shot pass; facade re-arms after read/commit | polling consumers primary; hybrid stays live; callback-driven loop is Phase 6 |
| Sync restarts after every committed event | boundary invalidates alignment by definition |
| Failed inputs excluded until refreshed (non-sticky via new descriptor) | avoids park-commit-park loops on permanently broken inputs |
| Effective-rate equality vs main (equal fields fast path; delta=2 @ 1/2 == delta=1 @ 1/1), ticks scaled onto the main lattice | rate is what matters; off-lattice ticks are InvalidDomain; dividers (rate multiples) stay Phase 5.5 |
| 2s sync timeout + 2s max range distance, hardcoded | pinned; parameterize later if needed |
| Gaps become boundaries: Event with `Gap` error, resync at commit while the input is used | accepted 2026-08-26; surfacing matters, realignment falls out of the commit-restarts-sync rule |
| Origin rule: matching origin type (absolute vs relative), epochs parsed and folded into tick offsets | accepted 2026-08-26; the equal-string requirement was too strict |
| availableCount: min over used+unfailed, gated on synced | "may lag reality, never exceed it" |

## Progress

- **Phase 1 — scaffolding** ✅: interfaces, params, status, facade wiring (lock-free), manager ingest/wake protocol, reconfigure semantics, gates, 32 tests.
- **Phase 2 — event pipeline** ✅: producer descriptor cache w/ delta merge, versioned delivery, real `read` (handshake/re-report) + `commitEvent`, disconnect boundaries + waiting state, `Disconnected` error, bootstrap via `enqueueLastDescriptor`, re-arm on consumption.
- **Phase 3 — available count** ✅: stagedSamples accounting in drain/delivery/discards, min-scan `getAvailableCount`.
- **Phase 4 — synchronization** ✅: domain parsing + local/relational validation, incremental main-anchored alignment, 2s timeout, range-distance guard, event-during-sync, failure exclusion. All 48 MultiReader2 tests green; full reader suite 2060/2060.
- **Phase 5 — data path + callback consumers** ✅: converted sample copies from the sync start, packetOffset, readWithDomain, minReadCount, gap boundaries (surfaced + resync), queue-based ready bits with consume-until-drained wakes, sync-deadline producer wakes, exported `MultiReader2`/`MultiReader2Params` factories, integer sample-rate validation, port adoption without re-owning.
- **Phase 6 — sum FB migration** ✅: `sum_reader_fb` rewritten on `IMultiReader2` (spare unused port, `BadInputHandling` Exclude/Deactivate property, reconfigure on connect/disconnect/property swap, clean-handshake recovery evidence, zeroed-buffer summing); 9-test functional suite drives the reader end to end through the block. Module suite 81/81, reader suite 2068/2068 green.

## Remaining work

- **Epoch parsing** (accepted 2026-08-26, next up): parse ISO-8601 origins and fold the epoch
  difference into the tick scaling so inputs with different absolute origins align; require a
  matching origin *type* (absolute vs relative), not equal strings. Deliberately sequenced
  after the sum FB - the FB reacts to per-input errors generically, so mixed origins already
  degrade honestly (InvalidDomain -> parked input).
- **Alignment tolerance decision** - see open questions; the sum FB dogfooding should inform it.
- Backlog: rtgen bindings, acceptsSignal veto policy, >64 inputs, read timeouts,
  `Disconnected`-vs-`setUsed` interplay polish, reactivation outside the event window.

## Out of scope (parked)

- **Dividers / multi-rate reads** - removed from the plan 2026-08-26; effective-rate
  equality with tick scaling onto the main lattice stays.
- Data-loss / producer-liveness monitoring (also removed on the remove-ladder-system branch).

## Size comparison

Three implementations of the same component, counted over every file exclusive to the multi
reader including its helper headers. `raw` is every line; `code` excludes blank and
comment-only lines.

| | files | headers raw/code | sources raw/code | **total raw** | **total code** |
|---|---|---|---|---|---|
| Old multi reader (`main`) | 11 | 1502 / 738 | 2670 / 2172 | **4172** | **2910** |
| Rework (`refactor/remove-ladder-system`) | 30 | 3883 / 1698 | 6841 / 5067 | **10724** | **6765** |
| MultiReader2 (this branch) | 13 | 778 / 354 | 1756 / 1425 | **2534** | **1779** |

MultiReader2 is 61% of the old reader and 26% of the rework by code lines. The rework is 2.3x
the reader it replaces.

Files counted — old: `multi_reader{,_builder,_builder_impl,_impl,_status}.*`,
`multi_typed_reader.h`, `signal_reader.{h,cpp}`, `reader_domain_info.h`. Rework: the same public
set plus `multi_reader_status_builder*`, `domain_value.h` (435), `enum_flags.h`,
`typed_reading_utils.{h,cpp}` (804), and the nine-header/seven-source `multi_reader/` directory.
MultiReader2: `multi_reader2*.{h,cpp}` plus `multi_reader_data_manager.{h,cpp}`.

Where the rework's bulk sits: `multi_reader_impl.cpp` 2291, `queue_reader.cpp` 1011,
`state_machine.cpp` 988, `typed_reading_utils.cpp` 725, `synchronization_manager.cpp` 618,
`domain_value.h` 435, `input.cpp` 397. MultiReader2's equivalents: `multi_reader_data_manager.cpp`
1094 and `multi_reader2_impl.cpp` 441. There is no separate typed-reading unit here — conversion
is 60 lines inside the manager — and no domain-value type family.

Tests, for scale: old 5467 lines in one file; rework 8584 across ten files (4245 public, 4339
white box, plus a 568-line benchmark); MultiReader2 4018 across four files (1284 unit, 2018
migrated public + white-box ports, 347 catalogue, 369 sum-FB functional). Test-to-code ratio:
1.9 old, 1.3 rework, 2.3 here.

## Differences vs the remove-ladder-system rework

Recorded 2026-08-26 against `refactor/remove-ladder-system` (openDAQ_Tomaz clone) and refined
by porting its test suites. Grouped below into behaviour differences (both readers do the job,
differently), issues (defects in MultiReader2 found by the migration), missing features (theirs
has it, we do not), and new features (ours has it, theirs does not).

### Behaviour differences

1. **Event-first versus data-first.** The rework serves samples staged in front of a descriptor
   boundary before reporting the event, and bounds `getAvailableCount` at the boundary.
   MultiReader2 zeroes availability the moment an event is queued anywhere and discards the
   staged samples on delivery. Ported as `DropOutdatedPacketSegments` and
   `AvailabilityStopsAtEventBoundary`, both asserting our answer; the divergence is FINDING 16.
2. **Connect on an unused input.** The rework answers it without disturbing alignment — an
   excluded input cannot change a model built from the used ones. MultiReader2 treats every
   connect as a wiring change and parks, so a mid-block reader stops to be reconciled. Ported
   inverted as `ConnectingAnUnusedInputParksTheReader`.
3. **Scope of a disconnect.** The rework discards only the disconnected slot's queue.
   MultiReader2 restarts synchronization, so every surviving input's unread block goes too.
   Ported widened as `DisconnectDiscardsWhatTheSlotHasAdopted`.
4. **When a bad domain is diagnosed.** The rework classifies at model-build time and names the
   input immediately. MultiReader2 classifies during synchronization, so the error surfaces one
   read after the descriptor event is committed — and for a permanently missing domain
   descriptor, only when the 2 s deadline expires (FINDING 18).
5. **Sync failure and buffered data.** Theirs keeps the failed input's buffered data; we discard
   it.
6. **Mutation model.** One path here — params plus `configure`, a full reset — versus five
   incremental mutators with bespoke invalidation.
7. **Event application.** Transactional here: park, `setUsed`/`setActive` inside the window,
   `commitEvent`, with the same status re-reported until committed. Theirs applies events at
   read time and signals through a `count == 0` handshake convention.
8. **Failure reporting.** Machine-readable per-input error codes here; an `InputState` enum plus
   a human diagnostic string there.
9. **Terminal state.** Theirs has a `markAsInvalid` no trigger can leave. Every failure here is
   per-input and recoverable through a fresh descriptor.
10. **Descriptor ownership.** Versioned merged cache here, so descriptor correctness is
    decoupled from queue survival. Theirs keeps event packets alive in the queues with exact
    O(1) connection counters.
11. **Wake model.** Transition-triggered bitmasks and a single `armed` flag exchanged to elect
    one waker, versus ready/used counters and a pass-epoch parity guard.
12. **Blocking.** `read` never blocks here; theirs supports read timeouts.

### Issues found (all recorded as tests, none fixed — validation pass only)

| # | Issue | Evidence |
|---|---|---|
| 1 | The sum block configures its output only when every input is healthy, so one bad input silences a block that could still sum the rest | `test_fb_sum.cpp` (DISABLED) |
| 2 | An invalid **main** input is never surfaced: every committed event restarts sync, so the main input's own failure is reported as "waiting" forever | `test_fb_sum.cpp` (DISABLED) |
| 3 | Gaps are counted, not queued, so the boundary position is unknown and post-gap data in the same window is discarded with the pre-gap data | `test_fb_sum.cpp` (DISABLED) |
| 3b/4 | Consequences of 3: recovery after an offset jump is racy (~1 run in 3 fails) | `test_fb_sum.cpp` (DISABLED) |
| 5 | Mixed epochs cannot align — origin strings are compared, never parsed | `DISABLED_MixedEpochsAlign` |
| 6 | Mixed tick resolutions are rejected unless the effective rate matches exactly | `DISABLED_MixedResolutionsAlign` |
| 7 | `read(nullptr, &count)` — every existing consumer's event probe — throws `ArgumentNull` | `DISABLED_NullBufferEventProbe` |
| 8 | A callback-only consumer is never woken: the notification `Work` is single-shot and discards `armDataAvailable()`'s "another pass is owed" return, so the reader latches disarmed | `DISABLED_MultiReaderOnReadCallback`, `DISABLED_RequestDuringEvaluationSchedulesFollowUp` |
| 9 | `setActive` is legal only inside an event window; the old reader accepted it any time | `MultiReaderActive` (adapted) |
| 10 | `getAvailableCount` returns 0 until a read has run — synchronization is driven only from `read` — so the classic `if (available >= n) read()` loop never starts. With 8, neither standard consumption pattern works unaided | `DISABLED_AvailableCountBeforeFirstRead`; every ported availability assertion needs a `syncByRead` first |
| 11 | `Config::requireSameRates` is written by the facade and never read: equal rates are unconditional, so the parameter is a lie | verified by grep; `ModelEqualRatePolicyViolation` is the test that would cover it |
| 12 | `clear()` has zero callers and resets staged data but not `synced`, `syncStarted`, `nextReadTick` or `deliveredGaps` — a cleared reader still believes it is synchronized against a stale start tick | dead code with a latent bug |
| 13 | `getAvailableCount` ignores `minReadCount`, so it can promise N samples `read` will then refuse | compounds 10 |
| 14 | Three unguarded `Int` multiplications in the sync path (rate comparison, distance bound, tick scaling) overflow silently at nanosecond resolution with a large scaling factor | their `CheckedArithmetic` / `RealisticTimeStamp` guard exactly this |
| 15 | `parseDomain` never inspects `getDimensions()`, so a dimensioned **domain** descriptor is accepted, while `convertibleValue` rejects dimensioned values — an asymmetry that looks like an oversight | their `DomainWithDimensionsRejected` |
| 16 | Event-first policy discards buffered data in front of a boundary and zeroes availability (see behaviour difference 1) | `DropOutdatedPacketSegments`, `AvailabilityStopsAtEventBoundary` |
| 17 | A fresh status object is allocated on every read; theirs re-issues one instance while the visible content is unchanged, so a polling consumer allocates once per poll | `StatusCachedWhileUnchangedNewOnChange` |
| 18 | A permanently missing domain descriptor is diagnosed 2 s late rather than immediately | `ModelMissingDomainDescriptor` |

Findings 11–15 are code defects found by mapping their tests onto our internals rather than by
running anything; each was verified in the source. Findings 8 and 10 are the two that block
real consumers and should be fixed first.

### Missing features (rework has, MultiReader2 does not)

- Multi-rate reads: LCM common rate, `requiredCommonSampleRate`, per-input dividers, block
  quanta, `startOnFullUnitOfDomain`. Deliberately cut 2026-08-26.
- Cross-domain unification: parsed ISO origins, earliest-common epoch, rational-GCD common
  resolution, absolute-time distance checks. Epoch parsing is accepted but not built.
- Alignment tolerance: half-block attributability, latched sync target. We require exact
  lattice hits.
- Read timeouts, `skipSamples`, `Scaled`/`Unscaled`/`RawValue` modes, auto-resolved `Undefined`
  read type, per-descriptor pre-resolved conversion functions.
- Any-rank values (vector, matrix, struct) and explicit domain rules.
- A `DomainValue`/`DomainInfo` type family able to express a sample's absolute time.
- Per-signal domain outputs; `getCommonSampleRate`/`getOffset`/`getTickResolution`/`getOrigin`
  accessors; a synthesized main-descriptor event packet; a human-readable diagnostic message.
- `MultiReaderFromExisting`, `dispose()`, configurable notification method.
- Cached status snapshots (finding 17) and a benchmark suite.

### New features (MultiReader2 has, rework does not)

- A transactional event window: the reader parks on an event, `setUsed`/`setActive` are legal
  only there, and `commitEvent` is the single point where wiring and synchronization restart.
  Nothing else in openDAQ's readers has an atomic reconfigure point.
- Machine-readable per-input error codes (`SyncFailed`, `DataLoss`, `Gap`, `InvalidDescriptor`,
  `InvalidDomain`, `Disconnected`) instead of a state enum plus prose.
- A single mutation path: an immutable params object plus `configure`, which cuts the wiring,
  mutates under lock, rebuilds, and re-bootstraps outside the lock.
- `readWithDomain` returning Int64 main-input ticks, and `packetOffset` from `read`, so a
  consumer can place a block on the timeline without a domain buffer.
- A versioned merged descriptor cache: the producer folds deltas into full-state packets, so
  the consumer never depends on an event packet surviving in a queue.
- Explicit `used` semantics as a first-class per-input concept, so a block with dynamic ports
  can exclude an input without disconnecting it (this is what the sum FB's `BadInputHandling`
  property switches between).
- Integer-sample-rate validation (`resolutionDen % (resolutionNum * delta) == 0`) that rejects
  domains no consumer could align, up front.
- A lock-free producer path: `addPacket` takes no lock, publishing into an SPSC dummy-node
  queue and electing a single waker by `armed.exchange(false)`.

## Test migration results

The public suite on `main` has 107 named cases; 33 port live, 74 are catalogued as out of scope
or regressions in `test_multi_reader2_unsupported.cpp` (43 of those are the reference-domain
family, which MultiReader2 has no notion of at all). The rework branch's public suite has 72
cases — 60 shared with main, 12 new — of which 5 are ported.

The rework's 117 white-box and utility cases were evaluated one by one: 1 maps directly, 60 can
be adapted through the public API, 5 are expressible but currently fail, and 51 need a feature
that does not exist here. 15 are ported. The reason only one maps directly is structural: their
tests target six classes (`QueueReader`, `Input`, `CallbackGate`, `NotificationCoordinator`,
`SynchronizationManager`, `ReadCoordinator`) and MultiReader2 has no white-box seam —
`MultiReaderDataManager`'s public surface is twelve methods, every helper is private, and there
is no `friend` and no test hook.

Current state: reader suite 2116/2116 green with 8 disabled findings; module suite 81/81 with 4
disabled. Suites: `MultiReader2Test` 28, `MultiReaderDataManagerTest` 28,
`MultiReader2MigrationTest` 54.

## Notes / quirks

- PATH cmake 3.29 too old for the build cache — use the VS-bundled cmake (path above).
- `DAQ_MAKE_ERROR_INFO` is printf-style: `%s` + `getCharPtr()`, never fmt braces.
- ObjectPtr has no `operator bool` — `while (auto p = pop())` compiles into a throwing value conversion; use `.assigned()`.
- `PacketPtr::Adopt(x).asPtr<T>(true)` dangles — keep the adopted ptr alive past the borrow.
- Test binary has a leak detector: never capture the reader strongly in its own event handler; `daqClearErrorInfo()` after every expected-error assertion.
- NullContext has no scheduler → notification passes run inline; a handler that reconfigures unconditionally recurses through the new bootstrap (guard test handlers).
- Never write wider samples than the value descriptor's type into a packet (a Float64 fill on an Int32 packet is a heap overrun).
- Property-write callbacks already hold the object lock (`getAcquisitionLock2`'s mutex) — do not re-take it there; reader event handlers come from outside and must take it.
- Module builds: `-DDAQMODULES_REF_FB_MODULE=ON -DDAQMODULES_REF_FB_MODULE_ENABLE_RENDERER=OFF -DDAQMODULES_REF_FB_MODULE_ENABLE_EXAMPLE_APP=OFF`; target `test_ref_fb_module`.

## Open questions

- **Alignment tolerance**: we currently require EXACT tick alignment on the main lattice
  (off-lattice tick or phase-shifted grid → `InvalidDomain`). The remove-ladder-system
  reader prefers an exact common tick but accepts inputs within half a block of the start
  (the phase offset stays visible in that signal's own domain output). Do we adopt a
  tolerance, and if so which — half-sample attributability or a configurable window?
- `getStatus` value when errors are present but data is readable (lean: Data, with errors dict populated).
- Whether `requireSameRates` survives as a param or becomes a validation shortcut.
- Struct/dimension value support (old spec allowed fixed-size block reads) — out of scope until conversion lands.

Resolved: `readWithDomain` timestamps are Int64 main-input ticks (decided with Phase 5).

## Note on tick alignment (musing, not a plan item)

With decimal sample rates and epochs always rounded to a full second, tick misalignment
cannot occur: every input's samples land on a common decimal sub-second grid, so the exact
lattice requirement costs nothing. Non-decimal rates are the problematic case. The domain
validation enforces the preconditions this relies on: the linear rule delta must be a whole
number and the sample rate (resolution denominator / (numerator * delta)) must divide out to
a whole number of samples per second.
