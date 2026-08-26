/*
 * Catalogue of multi reader test cases that MultiReader2 cannot host.
 *
 * Part one covers the public MultiReaderTest suite on main (test_multi_reader.cpp); part two
 * covers the white box and utility suites on refactor/remove-ladder-system.
 *
 * Each group names the old tests, the feature they need, and whether the gap is a deliberate
 * scope decision or an unclosed regression. Nothing here executes: the cases are recorded so
 * the cost of the new reader's narrower contract stays visible and countable.
 *
 * Counts refer to the 107 named tests in the original suite (108 declarations, 109 runnable:
 * one TEST_P over two values, one TEST_F_UNSTABLE_SKIPPED). Of those, 33 are ported live in
 * test_multi_reader2_migrated.cpp and 43 are the reference-domain family below.
 */
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// 1. Reference domain info - 43 tests - DELIBERATE (never in MultiReader2's scope)
//
//    ReferenceDomainIdEquality01..05, ReferenceDomainIdInequality01..06,
//    ReferenceDomainIdEqualityReferenceTimeProtocolEquality01..04,
//    ReferenceDomainIdEqualityReferenceTimeProtocolInequality01..15,
//    ReferenceDomainIdInequalityReferenceTimeProtocolInequality01..14
//
//    These assert that signals from different clock domains (reference domain id, time
//    protocol, offset) are rejected or accepted per a compatibility matrix. MultiReader2 has
//    no notion of reference domain info at all: it validates unit, rule, resolution, delta and
//    origin only. Two signals from unrelated clocks that agree on those fields would be
//    accepted and silently misaligned. This is the largest single behavioural gap.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 2. Multi-rate reads - 3 tests - DELIBERATE (dividers cut from the plan 2026-08-26)
//
//    SampleRateDivider, SampleRateDividerRequiredRate, StartOnFullUnitOfDomain
//
//    Inputs at different sample rates read as one block, counts expressed in common-rate
//    samples, per-input dividers. MultiReader2 requires every input to match the main input's
//    effective rate; anything else is InvalidDomain.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 3. Read timeouts - 5 tests - DELIBERATE (no blocking reads by design)
//
//    SignalStartDomainFrom0Timeout, SignalStartDomainFrom0TimeoutExceeded,
//    MultiReaderBuilderFromSignalsTimeouts, MultiReaderTimeoutWhenDataAvailable,
//    MultiReaderTimeoutChecking
//
//    MultiReader2::read never blocks; consumers poll or use onDataAvailable. Anything the
//    old timeout provided (wait for N samples) becomes the consumer's loop.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 4. skipSamples - 1 test - DELIBERATE (no skip API)
//
//    SignalStartDomainFrom0SkipSamples
//
//    Discarding without copying has no equivalent; a consumer reads and drops. Cheap to add
//    later (the read planner already advances without copying during sync).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 5. Read modes and read types - 3 tests - DELIBERATE
//
//    SignalStartDomainFrom0Raw (ReadMode::RawValue), UndefinedReadWithMockSignals,
//    UndefinedValueType (SampleType::Undefined resolved per input)
//
//    MultiReader2 has one scaled conversion path and requires an explicit value read type.
//    Undefined is rejected by the params object rather than resolved from the signal.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 6. Builder surface - 4 tests - DELIBERATE (params + configure replaced the builder)
//
//    MultiReaderBuilderGetSet, BuilderNotificationMethodsUnspecified, BuilderNotificationMethodDefault,
//    BuilderNotificationMethodsOverride
//
//    Notification method is not configurable: MultiReader2 forces SameThread on every input.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 7. Tick offset tolerance - 2 tests - DELIBERATE (deprecated on both branches)
//
//    TestTickOffsetExceeded, TestTickOffsetExceededByOffset
//
//    The remove-ladder-system branch deprecated and ignores this setting too.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 8. Epoch handling - 5 tests - REGRESSION, accepted for repair (epoch parsing is planned)
//
//    SignalStartDomainFrom0 (original form, three distinct epochs), MaxTimeIsNotOnSignalWithMaxEpoch,
//    Clock15MHzFromEpoch, EpochChanged, EpochChangedBeforeFirstData
//    (SignalStartRelativeOffset0 is ported live: a blank origin needs no conversion)
//
//    The old reader converted every input to absolute time and aligned across epochs.
//    MultiReader2 compares origin strings, so any epoch difference fails the input. A live
//    port of the first case is DISABLED_MixedEpochsAlign in test_multi_reader2_migrated.cpp.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 9. Tick resolution differences - 2 tests - REGRESSION, undecided
//
//    ResolutionChanged, Clock10kHzDelta10WithIntersampleOffset
//    (Clock10kHzDelta10WithAlignedOffset and ...Relative are ported live: one resolution each)
//
//    The old reader folded differing resolutions into a rational-GCD common resolution.
//    MultiReader2 accepts a different resolution only when the effective rate matches and every
//    tick lands on the main lattice. Live port: DISABLED_MixedResolutionsAlign.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 10. Sample rate changes mid-stream - 1 test - PARTIAL
//
//    SampleRateChanged
//
//    The change is reported as an event, but the new rate must still equal the main input's
//    or the input is dropped as InvalidDomain rather than the reader re-deriving a common rate.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 11. Reader lifecycle - 4 tests - DELIBERATE (no reader-to-reader migration)
//
//    ReuseReader, MultiReaderFromExisting paths, MultiReaderActiveCopyInactive,
//    DisposeDisconnectsInternalPorts
//
//    MultiReader2 has no MultiReaderFromExisting and no dispose(); configure() is the reset
//    path and the reader releases its ports on destruction. MultiReaderExceptionOnConstructor
//    is covered by MultiReader2Test construction tests instead.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 12. Gap detection - 2 tests - PARTIAL (gaps surface, realignment differs)
//
//    DISABLED_MultiReaderGapDetection (already disabled upstream), MultiReaderActiveGapPacket
//
//    MultiReader2 reports a gap as a boundary event with a Gap error and resynchronizes on
//    commit; it does not deliver the gap packet itself, and FINDING 3 (see test_fb_sum.cpp)
//    records that post-gap data in the same read window is discarded with the pre-gap data.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 13. Miscellaneous - 4 tests
//
//    ExpectSR, OffsetToLinear, TestReaderWithConnectedPortConnectionEmpty,
//    TestReaderWithConnectedPortConnectionNotEmpty
//
//    getCommonSampleRate / getOffset / getTickResolution / getOrigin have no equivalent on
//    IMultiReader2: the status carries the main input's domain descriptor and read() returns
//    the packet offset, so the information is reachable but the accessors are gone.
//    The connected-port-with-queued-packets cases are covered by the bootstrap handshake tests.
// ---------------------------------------------------------------------------

// ===========================================================================
// PART TWO - white box cases from refactor/remove-ladder-system
//
// That branch tests six internal classes directly: QueueReader, Input, CallbackGate,
// NotificationCoordinator, SynchronizationManager and ReadCoordinator. MultiReader2 has no
// equivalent seam - MultiReaderDataManager's public surface is twelve methods and every helper
// is private, with no friend declaration and no test hook - so none of those tests compile
// against this reader. 117 white box and utility cases were evaluated:
//
//    1 direct     UnconnectedSlotHasNothingAvailable, the only case whose assertion needs
//                 nothing but a reader with an unconnected input
//   60 adapt      the contract still exists here and can be re-expressed through the public
//                 API; 15 are ported in test_multi_reader2_migrated.cpp
//    5 public     expressible publicly but asserting something MultiReader2 does not do
//   51 unportable listed below, grouped by the feature that is missing
//
// The 15 ports are: InvalidDomainAndBack, DropOutdatedPacketSegments,
// AvailabilityStopsAtEventBoundary, NoUsedSlotsMeansNotReady, LinearRuleFindDomain,
// ExplicitRuleReadData (moved to the value side as PartialPacketReadsAtEveryOffset),
// SyncNeedMoreDataThenSynchronized, ModelMissingDomainDescriptor, NoSchedulerRunsInline,
// QueuedTaskOutlivesCoordinator, RequestDuringEvaluationSchedulesFollowUp (DISABLED, FINDING 8),
// ConnectingAnUnusedInputLeavesTheReaderSynchronized and DisconnectDiscardsWhatTheSlotHasAdopted
// (both inverted), plus the public MainInputDisconnectedWaits and
// StatusCachedWhileUnchangedNewOnChange.
// ===========================================================================

// ---------------------------------------------------------------------------
// 14. Dividers, block rounding and skip - 8 tests - DELIBERATE (cut from the plan 2026-08-26)
//
//    SyncManagerTest: ModelDeltaBasedDividers, SyncMixedRatesRoundsToBlockGrid
//    ReadCoordinatorTest: AvailabilityFloorsToBlockLcm, PlanRoundsRequestDownToBlocks,
//                         SkipSharesAlignmentWithRead, DiscardLeftoverSegmentsIsSilent
//    InputTest: MinimumConvertsToNativeRoundingUp
//    StateTransitions: RequiredRateNotDivisibleIsIncompatible
//
//    Every input reads at the main input's rate, so there is no divider, no common block and
//    no LCM to round to. Requests are honoured sample-exactly rather than floored to a block.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 15. Epoch parsing and absolute time - 5 tests - REGRESSION, accepted for repair
//
//    QueueReaderTest: OriginParsing, FirstSampleAbsoluteTime
//    SyncManagerTest: ModelEarliestEpochWins, SyncDifferentEpochsExact
//    DomainValueTest: RealisticTimeStamp
//
//    MultiReader2 compares origin strings and never parses them, so it can neither align
//    across epochs nor express a sample's absolute time. Group 8 above is the public face of
//    the same gap. RealisticTimeStamp additionally guards nanosecond-scale arithmetic against
//    Int64 overflow - see FINDING 14.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 16. The DomainValue / DomainInfo type family - 9 tests - DELIBERATE
//
//    DomainValueTest: DomainInfoComparison, StoresDaqInt, StoresDaqUInt, StoresDaqFloat,
//                     StoresDaqRange, Scaling, Offset, SameDomainSameType,
//                     NonRepresentibleConversions2
//
//    A 435 line header and a 255 line suite for a type-erased domain scalar that carries its
//    own DomainInfo and converts between representations. MultiReader2 works in main-input
//    ticks throughout: an Int64 and a rational scaling factor, no type family.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 17. Segment cursors and leftover discard - 4 tests - DELIBERATE (different queue model)
//
//    QueueReaderTest: DiscardLeftoverSegment, LeftoverSegmentPartialPackets,
//                     AdvancePastEnd, CheckAdvanceDomainEdgeCases
//
//    The rework branch models a packet as a segment with a cursor that can be advanced to an
//    arbitrary domain value, leaving a partially consumed leftover. MultiReader2 stages whole
//    packets in a deque with one front offset and has no advanceToDomainValue.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 18. Non-scalar value signals - 3 tests - DELIBERATE (scalar values only)
//
//    QueueReaderTest: VectorValueSignalLayout, MatrixValueSignalReadable, StructValueSignalReadable
//
//    MultiReader2 rejects a dimensioned or struct value descriptor as InvalidDescriptor.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 19. Half-block alignment tolerance - 3 tests - DELIBERATE (exact ticks required)
//
//    SyncManagerTest: SyncIntersampleOffsetWithinHalfBlockAccepted,
//                     SyncHalfSamplePeriodOffsetWithinHalfBlockAccepted
//    QueueReaderTest: AdvanceReachedValueBetweenTicks
//
//    The rework branch accepts a start offset up to half a block and rounds onto the grid.
//    MultiReader2 requires every input's ticks to land exactly on the main input's lattice;
//    anything else is InvalidDomain. Decided 2026-08-26 in favour of exactness.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 20. Required common sample rate and full-unit start - 3 tests - DELIBERATE
//
//    SyncManagerTest: ModelRequiredRateRefinesResolution, ModelRequiredRateNotDivisible,
//                     SyncStartOnFullUnitOfDomain
//
//    Neither setting exists on IMultiReader2Params. Group 2 above is their public face.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 21. Mixed tick resolutions in the common model - 2 tests - REGRESSION, undecided
//
//    SyncManagerTest: ModelMixedResolutions, RationalGcd
//
//    The rework branch folds differing resolutions into a rational-GCD common resolution.
//    MultiReader2 accepts a differing resolution only when the effective rate matches after
//    scaling. Group 9 above is the public face.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 22. CallbackGate counters and pass epochs - 6 tests - DELIBERATE (different wake model)
//
//    CallbackGateTest: RaiseIsIdempotentOnCounters, DisarmRetiresContributions,
//                      PassGuardTogglesEpochParity, ReadyToleratesStragglerOnLeavingUsedSet
//    NotificationCoordinatorTest: BeginOwnerPassMakesEpochNoisy, GateSharedAndStateChangeNotify
//
//    The rework branch counts ready and used slots and guards a pass with an epoch parity bit.
//    MultiReader2 uses transition-triggered bitmasks and a single armed flag exchanged to
//    elect one waker, so there are no counters to be idempotent about and no epoch to toggle.
//    FINDING 8 is the price: without the epoch there is nothing that records "another pass is
//    owed" across the end of a pass.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 23. Producer-side gate vocabulary - 4 tests - DELIBERATE
//
//    InputTest: ProducerForcesEvaluationInNonSteadyState,
//               ProducerRaisesReadyFromConnectionCountersWhenSteady,
//               ProducerGateIsDataFirstAcrossAnEventsJourney,
//               ProducerFallsBackToForceDuringOwnerPass
//
//    Steady state, force, owner pass and data-first are concepts of the rework branch's gate.
//    ProducerGateIsDataFirstAcrossAnEventsJourney is the one that matters: it pins the
//    data-first policy MultiReader2 inverts (FINDING 16).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 24. Explicit domain rules - 2 tests - DELIBERATE (implicit linear rule required)
//
//    TypedReadingTest: ExplicitRuleDomainReading, ExplicitRuleFindDomain
//
//    MultiReader2 needs a tick lattice it can compute; an explicit domain rule is InvalidDomain
//    (asserted by the ported InvalidDomainAndBack).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 25. Terminal Error state - 1 test - DELIBERATE (no terminal state)
//
//    StateTransitions: ErrorIsTerminalAndOutranksEverything
//
//    The rework branch has a markAsInvalid that no trigger can leave. Every MultiReader2
//    failure is per-input and recoverable through a fresh descriptor.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 26. Signal acceptance hook - 1 test - DELIBERATE
//
//    InputTest: AcceptsSignalForwardedAndDefaultAccept
//
//    MultiReader2 never installs an acceptsSignal handler on its ports.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 27. Non-representable domain conversions - 1 test - PARTIAL
//
//    DomainValueTest: NonRepresentibleConversions2
//
//    Guards conversions that cannot round-trip. MultiReader2 has one Int64 tick path, so most
//    of the matrix is unreachable; the arithmetic that remains is unguarded (FINDING 14).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 28. Public-only cases - 5 tests - expressible here, but currently failing
//
//    QueueReaderTest: DomainWithDimensionsRejected     -> FINDING 15 (never validated)
//    InputTest: InputIdFromSignalOrPort
//    SyncManagerTest: ModelEqualRatePolicyViolation    -> FINDING 11 (requireSameRates is dead)
//    SyncManagerTest: MainInputDefinesGridPhase
//    StateTransitions: SynchronizationDistanceExceededAndRemedy
//
//    These need no new feature, only behaviour MultiReader2 does not currently have. They are
//    the cheapest tests to make pass once findings 11 and 15 are addressed.
// ---------------------------------------------------------------------------

// Keeps the translation unit non-empty and the catalogue discoverable from a test run
TEST(MultiReader2Unsupported, SeeFileCommentsForTheCatalogue)
{
    SUCCEED() << "74 of 107 public and 51 of 117 white box cases are catalogued here as out of scope or regressions";
}
