/*
 * Catalogue of MultiReaderTest cases (test_multi_reader.cpp) that MultiReader2 does not host.
 *
 * Nothing here executes. Each group names the old tests, the feature they need, and why the new reader does not
 * have it, so the narrower contract stays visible and countable. The live ports are in
 * test_multi_reader2_migrated.cpp.
 */
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------------------------------------------
// 1. Reference domain info - 43 tests - out of scope
//
//    ReferenceDomainIdEquality01..05, ReferenceDomainIdInequality01..06,
//    ReferenceDomainIdEqualityReferenceTimeProtocolEquality01..04,
//    ReferenceDomainIdEqualityReferenceTimeProtocolInequality01..15,
//    ReferenceDomainIdInequalityReferenceTimeProtocolInequality01..14
//
//    These check that signals from different clock domains (reference domain id, time protocol, offset) are
//    accepted or rejected per a compatibility matrix. MultiReader2 validates unit, rule, resolution, delta, rate
//    and origin only. Two signals from unrelated clocks that agree on those fields are accepted and aligned on
//    their origins alone.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 2. Different sampling rates - 3 tests - Later (spec: resampling, sample rate dividers)
//
//    SampleRateDivider, SampleRateDividerRequiredRate, StartOnFullUnitOfDomain
//
//    Inputs at different rates read as one block with per-input dividers. MultiReader2 accepts equal rates only;
//    another rate is DomainDescriptorInvalid.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 3. Read timeouts - 5 tests - Later (spec: read with timeout)
//
//    SignalStartDomainFrom0Timeout, SignalStartDomainFrom0TimeoutExceeded, MultiReaderBuilderFromSignalsTimeouts,
//    MultiReaderTimeoutWhenDataAvailable, MultiReaderTimeoutChecking
//
//    A MultiReader2 read never waits; consumers read in a loop or from onDataAvailable.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 4. Read modes - 1 test - not in the design
//
//    SignalStartDomainFrom0Raw (ReadMode::RawValue)
//
//    MultiReader2 reads scaled values; Undefined reads every input as-is in its own (scaled) sample type.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 5. Builder surface - 6 tests - params and configure replaced the builder
//
//    MultiReaderBuilderGetSet, MultiReaderBuilderWithDifferentInputs, BuilderNotificationMethodsUnspecified,
//    BuilderNotificationMethodDefault, BuilderNotificationMethodsOverride, MultiReaderExceptionOnConstructor
//
//    The notification method is not configurable: every input port runs in Scheduler mode.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 6. Tick offset tolerance - 2 tests - not in the design
//
//    TestTickOffsetExceeded, TestTickOffsetExceededByOffset
//
//    Inputs have to sit exactly on the main input's tick grid; an offset is DomainDescriptorInvalid.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 7. Fractional origins - 3 tests - MVP domain assumptions
//
//    SignalStartDomainFrom0 (original form with a .123 s origin), MaxTimeIsNotOnSignalWithMaxEpoch,
//    Clock10kHzDelta10WithIntersampleOffset
//
//    Origins are ISO 8601 on a full second; an origin with a fraction is DomainDescriptorInvalid. Resolutions may
//    differ only when delta and resolution give the same rate and every tick lands on the main grid; the clock
//    and resolution tests themselves (Clock15MHzFromEpoch, ResolutionChanged) are ported with full-second origins.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 8. Callbacks of the old reader - 4 tests - replaced by onDataAvailable and read statuses
//
//    MultiReaderOnReadCallback, MultiReaderFromPortOnReadCallback, MultiReaderActiveDataAvailableCallback,
//    SharedDomainDescriptorChangeInvalidatesReaderAcrossCallbacks
//
//    The old reader had setOnDataAvailable with a count and an external listener for port events. MultiReader2
//    has one wake event and reports everything through read.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 9. Copies of a reader - 2 tests - no second constructor
//
//    ReuseReader, MultiReaderActiveCopyInactive
//
//    MultiReaderFromExisting is replaced by configure on the same reader.
// ---------------------------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------------------------
// 10. Reader config surface - 3 tests - IReaderConfig is not implemented
//
//    OffsetToLinear, ExpectSR, DisposeDisconnectsInternalPorts (ported in part as PortsKeepTheirOwnerAndListenerChanges)
//
//    getIsValid, getCommonSampleRate, getTickResolution and getOrigin live on the read status instead.
// ---------------------------------------------------------------------------------------------------------------
