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

- **Descriptor events are cache-only, never queued.** Only `DATA_DESCRIPTOR_CHANGED` is accepted; other event packets (gaps included) are dropped until Phase 6. Change packets carry deltas, so the producer keeps plain per-slot value/domain copies (single-writer) and exchanges a rebuilt **full-state merged packet** into the atomic cache (`lastEventPacket`), then bumps `eventVersion` and `pendingEvents`. Ownership transfers only by `exchange` on both sides — no load+addRef UAF.
- **Data packets**: dropped while any used input is disconnected, when the reader is inactive, or the slot unused; otherwise queued. Shared words are touched only on empty→non-empty transitions.
- **Wake election**: `deliverable()` = not parked ∧ all used inputs connected ∧ (pendingEvents>0 ∨ (active ∧ all used slots ready)). One producer wins `armed.exchange(false)`; the facade schedules a coalesced notification pass on the openDAQ scheduler (inline single-shot fallback without one). `armDataAvailable` re-checks and reclaims the lost-wakeup window. The facade re-arms after `read`/`commitEvent` so consumption reopens the wake window.

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
7. Otherwise Data status, count 0 (data copying is Phase 5).

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

## Remaining phases

- **Phase 5 - data read path + callback consumers**: copy planning over staged packets
  (syncedStart anchor, frontOffset advance), sample-type conversion to ValueReadType
  (scaled values), `packetOffset` = aligned start tick in main ticks, `readWithDomain`
  (buffer 0 = timestamps, #inputs+1 buffers), `minReadCount` enforcement, and the
  **callback-driven notification loop** (the pass re-schedules itself while deliverable so
  `onDataAvailable` keeps firing for callback consumers - required by the sum FB).
  Also an **exported factory** for the reader/params (impl classes are not DLL-exported,
  so module code cannot instantiate them today).

  Sync extensions accepted from the remove-ladder-system comparison (2026-08-26):
  - **Epoch parsing**: parse ISO-8601 origins and fold the epoch difference into the tick
    scaling so inputs with different absolute origins align; require matching origin *type*
    (absolute vs relative), not equal strings.
  - **Gap events surfaced**: an `IMPLICIT_DOMAIN_GAP_DETECTED` packet becomes a boundary —
    read reports an Event with a `Gap` error for that input; at commit, if the gapped input
    is still used, synchronization restarts (a commit restarts sync anyway, so the rule
    costs nothing extra). Gaps on unused inputs are reported but do not force a resync.

- **Phase 6 - sum FB migration + functional tests**: port `sum_reader_fb` (ref_fb_module,
  currently on the remove-ladder-system branch) to `IMultiReader2`, strictly on the canonical
  usage pattern (dataAvailable -> read -> on Event: setUsed/setActive -> commitEvent -> read data):
  - Dynamic input ports: always one spare disconnected port, handed to the reader as an
    **unused** input via params.
  - A `BadInputHandling` property with two modes: `Deactivate` (reader-wide `setActive(false)`
    while inputs fail) vs `Exclude` (failing inputs get `setUsed(false)`); swapping the
    property **reconfigures** the reader.
  - `configure()` is called on every connect/disconnect (the port set changes) and on the
    property swap; every other reaction goes through the event window.
  - Functional test suite that exercises the multi reader end-to-end through the sum block:
    connect/disconnect churn, descriptor changes, sync failures under both handling modes,
    the spare unused port, property swaps mid-stream.

  Features currently missing for the port (notes accepted 2026-08-26, tracked as Phase 5/6 work):
  1. The data path itself + conversion (Phase 5) - hard prerequisite.
  2. Callback-driven dataAvailable loop (Phase 5).
  3. Exported factory - a module cannot `createWithImplementation` a non-exported impl.
  4. Port adoption without re-owning: the facade does `setOwner(portBinder)` on adopted
     ports; FB input ports already have the FB as owner - the facade must skip owner
     binding when the port has one.
  5. Listener handoff: the reader takes `setListener` on adopted ports, so the FB's
     `onConnected`/`onDisconnected` overrides never fire - the FB must subscribe to the
     reader's `getOnConnected`/`getOnDisconnected` events instead (works today; noted
     because it changes the usual FB pattern).
  6. Reactivation quirk: `setActive(true)` is only legal inside an event window; in
     `Deactivate` mode recovery therefore rides on the next event or on a reconfigure.
     Acceptable for the FB (the property swap reconfigures) but worth revisiting.

## Out of scope (parked)

- **Dividers / multi-rate reads** - removed from the plan 2026-08-26; effective-rate
  equality with tick scaling onto the main lattice stays.
- Data-loss / producer-liveness monitoring (also removed on the remove-ladder-system branch).
- rtgen bindings, acceptsSignal veto policy, >64 inputs, read timeouts.

## Notes / quirks

- PATH cmake 3.29 too old for the build cache — use the VS-bundled cmake (path above).
- `DAQ_MAKE_ERROR_INFO` is printf-style: `%s` + `getCharPtr()`, never fmt braces.
- ObjectPtr has no `operator bool` — `while (auto p = pop())` compiles into a throwing value conversion; use `.assigned()`.
- `PacketPtr::Adopt(x).asPtr<T>(true)` dangles — keep the adopted ptr alive past the borrow.
- Test binary has a leak detector: never capture the reader strongly in its own event handler; `daqClearErrorInfo()` after every expected-error assertion.
- NullContext has no scheduler → notification passes run inline; a handler that reconfigures unconditionally recurses through the new bootstrap (guard test handlers).

## Open questions

- **Alignment tolerance**: we currently require EXACT tick alignment on the main lattice
  (off-lattice tick or phase-shifted grid → `InvalidDomain`). The remove-ladder-system
  reader prefers an exact common tick but accepts inputs within half a block of the start
  (the phase offset stays visible in that signal's own domain output). Do we adopt a
  tolerance, and if so which — half-sample attributability or a configurable window?

- `readWithDomain` timestamp buffer type: main input's raw domain type as-is (current lean) or normalized.
- `getStatus` value when errors are present but data is readable (lean: Data, with errors dict populated).
- Whether `requireSameRates` survives as a param or becomes a validation shortcut.
- Struct/dimension value support (old spec allowed fixed-size block reads) — out of scope until conversion lands.
