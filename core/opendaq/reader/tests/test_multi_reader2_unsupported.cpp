/*
 * Catalogue of MultiReaderTest cases (test_multi_reader.cpp) that MultiReader2 cannot host.
 *
 * Each group names the old tests, the feature they need, and whether the gap is a deliberate
 * scope decision or an unclosed regression. Nothing here executes: the cases are recorded so
 * the cost of the new reader's narrower contract stays visible and countable.
 *
 * Counts refer to the 107 tests in the original suite.
 */
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// 1. Reference domain info - 60 tests - DELIBERATE (never in MultiReader2's scope)
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
// 6. Builder surface - 5 tests - DELIBERATE (params + configure replaced the builder)
//
//    MultiReaderBuilderGetSet, MultiReaderBuilderWithDifferentInputs,
//    BuilderNotificationMethodsUnspecified, BuilderNotificationMethodDefault,
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
// 8. Epoch handling - 6 tests - REGRESSION, accepted for repair (epoch parsing is planned)
//
//    SignalStartDomainFrom0 (original form, three distinct epochs), SignalStartRelativeOffset0,
//    MaxTimeIsNotOnSignalWithMaxEpoch, Clock15MHzFromEpoch, EpochChanged,
//    EpochChangedBeforeFirstData
//
//    The old reader converted every input to absolute time and aligned across epochs.
//    MultiReader2 compares origin strings, so any epoch difference fails the input. A live
//    port of the first case is DISABLED_MixedEpochsAlign in test_multi_reader2_migrated.cpp.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// 9. Tick resolution differences - 3 tests - REGRESSION, undecided
//
//    ResolutionChanged, Clock10kHzDelta10WithIntersampleOffset,
//    Clock10kHzDelta10WithAlignedOffsetRelative
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

// Keeps the translation unit non-empty and the catalogue discoverable from a test run
TEST(MultiReader2Unsupported, SeeFileCommentsForTheCatalogue)
{
    SUCCEED() << "97 of the 107 MultiReaderTest cases are catalogued here as out of scope or regressions";
}
