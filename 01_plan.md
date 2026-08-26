# MultiReader2 — Architecture and Implementation Plan

Branch: `refactor/multi-reader-2` (main openDAQ repo, based on `main`).
Sources: `core/opendaq/reader/{include/opendaq,src}/multi_reader2*`, `multi_reader_data_manager.*`; tests in `core/opendaq/reader/tests/test_multi_reader2.cpp`.
Build: VS-bundled CMake (`Visual Studio 18 2026` generator), build tree `build/x64/msvc-26/full`, all modules disabled, core + tests only. Target `test_reader`, config Release.

## 1. Architecture

### Component layout

```
IMultiReader2                    (public interface, no builder)
 └─ MultiReader2Impl             (facade: ImplementationOfWeak<IMultiReader2, IInputPortNotifications>)
     ├─ Slot[0..N-1]             (port ptr, input id, ownsPort)
     ├─ Wiring (snapshot)        (unordered_map<IInputPort*, {index, id}>; atomic shared_ptr)
     ├─ SchedulerPtr + WorkPtr   (coalesced dataAvailable dispatch)
     ├─ EventEmitters            (onConnected, onDisconnected, onDataAvailable)
     └─ MultiReaderDataManager   (plain C++, value member, full ownership)
         ├─ Config               (consumer-side: ids, used/connected flags, mainInputId,
         │                        valueReadType, minReadCount, requireSameRates)
         └─ State (snapshot)     (producer-visible; atomic shared_ptr)
             ├─ SlotCell[N]      (alignas(64): SPSC packet queue, dataPacketCount,
             │                    lastEventPacket descriptor cache)
             ├─ readyMask, usedMask, connectedMask   (atomic bitmasks, 64-input limit)
             ├─ queuedEventPackets, parked, active, armed

IMultiReader2Params              (input list, unused inputs, main input, valueReadType,
 └─ MultiReader2ParamsImpl        minReadCount, requireSameRates; validates homogeneity)

IMultiReader2Status              (getStatus Data|Event, getDomainDescriptor,
 └─ MultiReader2StatusImpl        getDescriptors, getDividers, getErrors; immutable value object)
```

### Threading model

- Producer paths (`packetReceived`, `connected`, `disconnected`) are lock-free: one atomic
  load of the Wiring snapshot, hash lookup, SPSC push, transition-only shared atomics.
- Consumer paths (`read`, `commitEvent`, `setUsed`, `setActive`, `reconfigure`, `clear`)
  share the manager's `consumerMutex`; the facade `mutex` guards `configure` and getters.
- No lock is ever held while a callback or event emitter fires. This is API contract:
  handlers may call `configure` re-entrantly (tested, including reconfiguring away the
  input whose sendPacket/connect/disconnect is on the stack).
- `configure` swaps the Wiring snapshot: cut (store null) -> mutate -> rewire (store new).
  `reconfigure` swaps the manager State the same way. In-flight callbacks finish safely
  on the old snapshot; their packets are discarded with it.

### Ingest and wake protocol

- Per-slot SPSC queue (dummy-node linked list; producer owns tail, consumer owns head,
  only `next` is atomic). One producer per input (SameThread notification, forced).
- `readyMask` bit set on a slot's empty->non-empty data transition; steady-state packets
  touch no shared word. Event packets count in `queuedEventPackets`.
- Wake condition (`deliverable`): not parked, all used inputs connected, and
  (an event packet is queued OR every used input has data). Inactive readers wake only
  for events.
- Wake election: `deliverable() && armed.exchange(false)` — exactly one producer wins.
  `armDataAvailable()` re-arms only after observing a clean condition, with a
  re-check-and-reclaim to close the lost-wakeup window.
- `dataAvailable` is dispatched through the openDAQ scheduler (one coalesced WorkPtr,
  weak self-ref); inline single-shot fallback when the context has no scheduler.
  Currently single-shot (polling consumers assumed); the re-schedule loop returns with
  callback-driven consumers.

### Ingest drop rules

| Condition | Data packets | Event packets |
|---|---|---|
| Normal (used, connected, active) | queued | queued + cached |
| Input unused | dropped | queued + cached |
| Reader inactive | dropped | queued + cached |
| Any used input disconnected | dropped | dropped, but cached |
| Event pending (parked) | queued | queued + cached (no wakes) |

`lastEventPacket` per slot always holds the newest event packet (single-writer atomic
swap), so descriptors survive every drop rule. Each cache carries a producer-incremented
version; the consumer tracks the last version delivered to the user, and any undelivered
cache is itself a window condition � correctness never depends on an event packet
surviving in a queue.

### Bootstrap sequence and descriptors-before-data

`connectInternal` calls `connected` before enqueueing the initial descriptor event
(same thread), so the last connect lifts the waiting gate before its descriptor
arrives: earlier descriptors are dropped-and-cached, the last one queues and wakes.
First read returns the handshake Event (queued descriptor + cached ones), commit
applies, the second read runs sync. Read check order � parked, boundary/undelivered
descriptor versions, sync, data � structurally enforces that descriptors always reach
the user before any data, even when sync could succeed immediately. The wake from the
last connect triggers the read, not sync.

### Event window (parked state)

Set by the read path on hitting an event boundary. While parked: `read` re-reports the
same Event status with no data, `getAvailableCount` is 0, no wakes fire, ingest continues
behind the boundary. `setUsed`/`setActive` are legal only inside the window (masks update
immediately; queue consequences execute at commit). `commitEvent` is the transaction
point: pop boundary events, apply descriptors, run staged queue surgery, unpark.
`configure`/`clear` are always legal and cancel the window (full reset) — reconfigure and
commitEvent are mutually exclusive by construction.

### Synchronization model (designed, not yet implemented)

Two layers: a common model (grid) and per-input alignment.

- Common grid = main input: its sample rate is the common rate, its origin is the origin,
  its domain descriptor is the reported output domain.
- Per-input divider = mainRate / inputRate, must be an integer >= 1; otherwise that input
  is flagged `InvalidDomain`. Faster-than-main inputs are always invalid (no resampling).
- Per-input alignment state {Unaligned, Aligned, Failed}; sync is lazy (at read start)
  and never sticky. `setUsed(false)` touches nothing else; `setUsed(true)` aligns only
  the re-joining input; the model recomputes only when a used input's rate/domain changes
  or a re-added input does not fit. A gap realigns only the gapped input.
- Sync is incremental across read calls, never blocking (read has no timeout param;
  consumers poll). During sync the read path exclusively owns the queues: pops discard
  toward the alignment point; ownership "hands back" = Gathering -> Streaming.
- `read` during sync: in progress -> count 0, status Data, no errors; failures decided ->
  status Event (parked!) with SyncFailed per input � sync failure opens a window because
  setUsed is window-gated; commit without changes = retry. Aligned -> same call reads.
- Failure triggers: (1) no new data within 2 s of sync start (hardcoded; lazy
  steady_clock check per read); (2) range distance to the MAIN input's range >
  maxSyncDistance (closest-two-timestamps rule, 0 on overlap; main-anchored so the
  failing input is well-defined; main itself never fails � that is the setActive story);
  (3) an event packet arriving mid-sync fails that input; the event stays queued and
  becomes the next window after commit.
- Per-slot [first, last] queued-timestamp range: last is producer-maintained at
  addPacket (from packet offset + count), first is the consumer-side queue head �
  the distance check is atomic loads only.
- Domain constraints: integer domain value types only (no conversion), unit must be
  "seconds", implicit linear rule only (explicit domains rejected), origins must match
  the main input (relative allowed only if main is relative); violations are per-input
  `InvalidDomain`.
- Common domain = main input origin + reference resolution, published into State by the
  main slot producer when its descriptor validates. A slot queues data only when the
  common domain is published AND its own cached descriptor is locally valid; until then
  data is dropped and descriptors cached (same predicate family as the Waiting state).
  At ingest the producer converts the packet end timestamp into common ticks and
  maintains the slot [first, last] range.
- Main input always succeeds in synchronization (at worst synced to itself): a
  single-input reader syncs instantly; sync can only shrink the delivering set, never
  fail globally.
- Sync failure sets the committable event flag (parked). Hard reason: equal-count
  delivery is impossible with a used-but-failed input, someone must decide, and the
  window is the FB's only mutation point. Pending stays the only state where the FB acts.
- Sync trigger: inside `read` for polling consumers; `dataAvailable` fires on the
  pre-sync raw condition, so the wake always precedes sync. Callback-consumer option
  (Phase 6): move the sync attempt into the scheduled pass so the callback fires only
  on a definitive outcome.

## 2. Decisions made

| Decision | Rationale |
|---|---|
| No builder; params object + `configure` is the only mutation path | One code path for construction and reconfiguration; `addInput`/`removeInput` removed as redundant |
| Constructor takes params and calls `configure`; failures throw via `checkErrorInfo` | Reader is fully wired from birth; interface methods return error codes, only the ctor throws |
| No constructor input-type decision; per-slot `ownsPort` instead | Params validate homogeneity; signals-vs-ports is a per-slot fact; kind can swap wholesale on reconfigure |
| `valueReadType` is mandatory (getter returns `OPENDAQ_ERR_NOTASSIGNED` until set) | Explicit user decision; `SampleType::Invalid == Undefined == 0` makes a default sentinel impossible |
| Main input: explicit or defaults to first; exposed resolved via `getMainInput`; cannot be unused | Defines the output grid, rate, origin, and reported domain descriptor |
| Unused inputs settable in params (`setUnusedInputs`) | Spare-empty-input pattern; unconnected spares must not gate reading |
| `reconfigure` is a full reset; queued data dropped, staged state cancelled | Users reconfigure in reaction to connect/disconnect; bootstrap re-derives everything from current port reality |
| Reconfigure legal anywhere, including inside `dataAvailable`/`onConnected`/`onDisconnected` | Escape hatch must always work; costs only invariants already required by the lock-free design (no locks across callbacks, snapshot swaps mandatory) |
| Status: `{Data, Event}` only; errors are a per-input dict (`getErrors`) | A separate Error status duplicated the dict; per-input errors: SyncFailed, DataLoss, Gap, InvalidDescriptor, InvalidDomain (+ Disconnected, pending) |
| Data and Event are not exclusive: `read` may return data up to the boundary plus Event status | One call instead of two; boundary data is never silently held back |
| `commitEvent` (renamed from consumeEvent) is the single transaction point | Staged setUsed/setActive apply at commit; parked reader makes immediate mask writes safe |
| Event args carry the slot id (event name field); connected/disconnected forward through emitters | Subscribers key everything by input id; sender is the port |
| All ports forced to `PacketReadyNotification::SameThread` | Bounded producer work + manager-side scheduling replaces port-side Scheduler dispatch |
| `dataAvailable` via openDAQ scheduler, coalesced, at most one task outstanding | Producer must never run user callbacks; armed flag doubles as task-coalescing gate |
| Index-keyed hot path (`addPacket(slotIndex)`), string-keyed cold path | Global-id string hashing per packet is waste; Wiring snapshot resolves port->index O(1) |
| Waiting-for-connection state: any used input disconnected silences all wakes, drops all packets, caches descriptors | Old reader's portsConnected gate, redone; recovery is either handler reconfigure or (pending) the read-path Event |
| Mixed origins rejected (`InvalidDomain`); main input's origin wins; no epoch reconciliation | Simplification over old reader; co-domain devices share origin strings in practice |
| Polling consumers assumed for now (`while read/commit` loop) | Arming stays single-shot; loop/re-schedule semantics return with callback-driven consumers |
| Per-slot `availableSamples` maintained producer-side; may lag, never exceed reality | O(1)-per-packet accounting; undercount self-heals, overcount would corrupt read plans |
| Dividers implemented last; interim rule: input rate must equal main rate | They only complicate the availability/plan math; everything else is divider-agnostic |
| 64-input limit (single-word masks), asserted | Widen to word arrays if ever needed |

## 3. Progress

Done (all building, 32 MultiReader2 tests + full reader suite 2038 green):

- Interfaces: `IMultiReader2` (configure, getMainInput, getAvailableCount, read,
  readWithDomain, commitEvent, setUsed, setActive, three event getters),
  `IMultiReader2Params`, `IMultiReader2Status` (+ enums). Ordered, doc-commented,
  rtgen templateType annotations in place (not yet registered with rtgen).
- Facade: configure diff (remove/add/reorder, validation before mutation), port
  creation/adoption, wiring snapshot, lock-free callbacks, scheduler dispatch,
  event forwarding with slot ids.
- Manager: lock-free ingest (SPSC + masks + wake election), drop rules, waiting-for-
  connection state, per-slot descriptor cache, full-reset reconfigure, event-window
  gates, consumer mutex.
- Tests: params validation, construction errors, reconfigure semantics, re-entrancy
  (configure inside onConnected/onDisconnected/onDataAvailable), gates, wake protocol,
  drop rules, disconnect silencing, concurrency (producer race, reconfigure under fire).

Shells only (return NOTIMPLEMENTED or 0): `read`, `readWithDomain`, `getAvailableCount`,
`commitEvent` (gate only), manager `read`.

## 4. Remaining steps

### Phase 2 — event pipeline end to end
- Consumer-side slot state: cached descriptors (seeded from `lastEventPacket`),
  queue pop discipline (pop -> count -> clear ready bit -> recheck -> re-set).
- Boundary detection in `read`: earliest event across used slots or a synthesized
  disconnect boundary -> build real status (Event, descriptors dict, errors dict),
  set parked, count = 0. Consecutive descriptor events merge (newest wins); nothing
  merges across gap/disconnect boundaries.
- `commitEvent`: pop boundary, apply descriptors, staged queue surgery
  (unused: drop data keep events; inactive: all slots; disconnected: whole queue),
  unpark.
- Add `Disconnected` to `MultiReader2InputError`; `connectionEventsPending` counter
  feeding boundary detection (not the wake path — polling model).
- Facade bootstrap: `enqueueLastDescriptor` on every connected port after `configure`
  releases the mutex (never under the lock — re-entrancy deadlock otherwise).
- Recovery from waiting state: on full reconnection the next read builds its Event from
  the descriptor caches plus fresh initial events.
- Exit: FB example's event half runs live (construct -> Event handshake -> setUsed/
  setActive -> commit -> no boundary).

### Phase 3 — availability
- Producer stores per-packet sample count in the SPSC node; per-slot atomic sample
  counter for O(1) `getAvailableCount` (min over used slots up to first boundary;
  divider-floored once Phase 4 lands).

### Phase 4 — common model and per-input alignment
- Domain parsing per pinned constraints (integer type, seconds, linear rule);
  rate extraction; divider computation vs main input; `requireSameRates` check
  (all dividers == 1).
- Rational tick mapping across differing resolutions (constraints do not remove this).
- Per-input alignment with the {Unaligned, Aligned, Failed} model; lazy, never sticky;
  per-input realignment on gap and on setUsed(true); model recompute only on used-input
  rate/domain change.

### Phase 5 — data read path
- Plan (min available, divider-scaled per-input counts, stop at boundary) -> copy into
  jagged buffers -> advance cursors -> `packetOffset` from main input.
- `readWithDomain`: materialize timestamps from the main domain into buffer 0.
- `minReadCount` honored; same-type copies first, conversion later.
- Revisit `packetOffset` type (`SizeT` cannot express negative or rational positions).

### Phase 5.5 � dividers (deliberately last)
- Divider = mainRate / inputRate (integer, else `InvalidDomain`); availability becomes
  divider-scaled min; plan counts scale per input; block alignment via divider LCM.

### Phase 6 — edges and polish
- Gap handling (per-input realign; `Gap` reported, input not evicted).
- `DataLoss` detection (arrival deadlines) if kept.
- Factory functions (params + reader), rtgen registration, bindings.
- Callback-driven consumer support: restore the pass re-schedule loop, define
  `onDataAvailable` handler contract (reconfigure conditionally, never unconditionally).
- Decide `acceptsSignal` forwarding / FB veto hook.

## 5. Open questions

- `readWithDomain` timestamp buffer type: main input's raw domain type as-is
  (current lean) or normalized.
- `getStatus` value when errors are present but data is readable (lean: Data, with
  errors dict populated).
- Whether `requireSameRates` survives as a param or becomes a validation shortcut.
- Struct/dimension value support (old spec allowed fixed-size block reads) — out of
  scope until conversion lands.
