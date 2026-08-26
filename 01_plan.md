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

## Differences vs the remove-ladder-system rework

Recorded 2026-08-26 against `refactor/remove-ladder-system` (openDAQ_Tomaz clone), the full
rewrite of the existing multi reader (~85 files, +20k lines: QueueReader per input,
SynchronizationManager, ReadCoordinator, NotificationCoordinator, lock-free callback gate;
currently mid-migration between two state machines).

What MultiReader2 gains:
- ~1/4 the code; one consumer mutex plus a producer path auditable in one function, versus a
  callback-gate/pass-epoch protocol whose data-first rule lives in four places and two live
  state machines.
- A transactional event contract: park -> setUsed/setActive inside the window -> commitEvent,
  with the same status re-reported until committed. Theirs applies events at read time and
  signals through a count==0 handshake convention.
- One mutation path (params + configure = full reset) versus five incremental mutators with
  bespoke invalidation.
- Descriptor correctness decoupled from queue survival (versioned merged cache); theirs must
  keep event packets alive in queues with exact O(1) connection counters.
- Machine-readable per-input error codes versus an InputState enum plus a diagnostic string.

What the rework has that MultiReader2 does not:
- Multi-rate reads (LCM/required common rate, dividers, block quanta) - deliberately cut here.
- Cross-domain unification: rational-GCD common resolution, earliest-common epoch with parsed
  origins and absolute-time distance checks (our epoch parsing is accepted but not yet built;
  we check distance in ticks).
- Alignment tolerance: half-block attributability, startOnFullUnitOfDomain, latched sync
  target; we require exact lattice hits.
- Read timeouts, skipSamples, Scaled/Unscaled/RawValue modes, auto-resolved Undefined read
  type, per-descriptor pre-resolved conversion functions, any-rank values, per-signal domain
  outputs.
- Status polish: synthesized main-descriptor packet, diagnostic message, allocation-free
  cached status snapshots; plus benchmarks backing micro-decisions.
- Sync failure there keeps the failed input's buffered data; we discard it.

Closed since the comparison was first made: the data path, conversion, minReadCount,
packetOffset, gap surfacing, callback-driven consumption, and the exported factories all
now exist here. Neither implementation monitors producer liveness or resamples.

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
