#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Policy : public MultiReader2Test
{
};

class MultiReader2PolicyEachCall : public EachReadingCall
{
};

// S1.POL.01
TEST_F(MultiReader2Policy, DefaultInvalidateReadsAllHealthyInputsWithoutChanges)
{
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
}

// S1.POL.02
TEST_F(MultiReader2Policy, ExcludeReadsAllHealthyInputsWithoutChanges)
{
    given().policy(Exclude);
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
}

// S1.POL.03
TEST_F(MultiReader2Policy, MainDomainWithoutOriginInvalidatesReaderAndDropsData)
{
    setupBase();
    announce(in1, Domain().noOrigin());
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .mainNull()
               .inError(in1, InputError::DomainDescriptorInvalid)
               .msg("in1: domain descriptor invalid, no origin"));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(avail(), count(0));
    expect(read(0), count(0).noChanges());
}

// S1.POL.04
TEST_F(MultiReader2Policy, NonMainOriginOffFullSecondInvalidatesReader)
{
    setupBase();
    announce(in2, Domain().originMs(500));
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .mainNull()
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, origin not an ISO 8601 time on a full second"));
}

// S1.POL.05
TEST_F(MultiReader2Policy, PinnedMainWithFloatDomainTypeInvalidatesReader)
{
    given().main(in2);
    setupBase();
    announce(in2, Domain().type(SampleType::Float64));
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, ticks not of an integer type"));
}

// S1.POL.06
TEST_F(MultiReader2Policy, NonPinnedInputWithDimensionedDomainInvalidatesReader)
{
    given().main(in2);
    setupBase();
    announce(in1, Domain().type(SampleType::Int64, {2}));
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .inError(in1, InputError::DomainDescriptorInvalid)
               .msg("in1: domain descriptor invalid, ticks not scalar"));
}

// S1.POL.07
TEST_F(MultiReader2Policy, ExcludeMovesAutomaticMainWhenMainDomainUnitNotSeconds)
{
    given().policy(Exclude);
    setupBase();
    announce(in1, Domain().unitMs());
    write(in2, 100, 199);
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .main(in2)
               .domainChanged()
               .resync()
               .descChanged(in2)
               .inError(in1, InputError::DomainDescriptorInvalid)
               .msg("in1: domain descriptor invalid, unit not seconds"));
    expect(avail(), count(0));
    write(in1, 200, 299);
    write(in2, 200, 299);
    expect(read(100), count(100).at(200).noChanges());
}

// S1.POL.08
TEST_F(MultiReader2Policy, ExcludeSetsAsideNonMainWithExplicitDomainRuleWithWarning)
{
    given().policy(Exclude);
    setupBase();
    announce(in2, Domain().explicitRule());
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, rule not linear"));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.POL.09
TEST_F(MultiReader2Policy, PinnedMainInErrorInvalidatesReaderUnderExclude)
{
    given().policy(Exclude).main(in2);
    setupBase();
    announce(in2, Domain().delta(0));
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, rule delta not an integer above 0"));
    write(in1, 100, 199);
    expect(avail(), count(0));
}

// S1.POL.10
TEST_F(MultiReader2Policy, ExcludeSetsAsideNonPinnedInputWithFractionalRuleStart)
{
    given().policy(Exclude).main(in2);
    setupBase();
    announce(in1, Domain().start(1.5));
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .inError(in1, InputError::DomainDescriptorInvalid)
               .msg("in1: domain descriptor invalid, rule start not an integer"));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.POL.11
TEST_F(MultiReader2Policy, SeveralMissingValueDescriptorsInvalidateWithMessageInSlotOrder)
{
    given().inputs(10);
    setupBase();
    announce(in3, Value::null());
    announce(in2, Value::null());
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .mainNull()
               .inError(in2, InputError::ValueDescriptorInvalid)
               .inError(in3, InputError::ValueDescriptorInvalid)
               .msg("in2: no value descriptor; in3: no value descriptor"));
}

// S1.POL.12
TEST_F(MultiReader2Policy, ExcludeMovesAutomaticMainPastSeveralInputsInError)
{
    given().inputs(10).policy(Exclude);
    setupBase();
    announce(in2, Value::null());
    announce(in1, Value::null());
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .main(in3)
               .domainChanged()
               .resync()
               .allDescChanged()
               .inError(in1, InputError::ValueDescriptorInvalid)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in1: no value descriptor; in2: no value descriptor"));
}

// S1.POL.13
TEST_F(MultiReader2Policy, AllPortsUnconnectedInvalidatesReaderAsDisconnected)
{
    given().ports().unconnected(in1).unconnected(in2);  // the row names no unconnected port; its result needs both
    expect(read(0),
           count(0)
               .level(Error)
               .mainNull()
               .inError(in1, InputError::Disconnected)
               .inError(in2, InputError::Disconnected)
               .msg("in1: not connected; in2: not connected"));
}

// S1.POL.14
TEST_F(MultiReader2Policy, ExcludeInvalidatesReaderWhenNoInputIsHealthy)
{
    given().policy(Exclude);
    setupBase();
    announce(in1, Domain::null());
    announce(in2, Domain::null());
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .mainNull()
               .inError(in1, InputError::DomainDescriptorInvalid)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in1: domain descriptor invalid, no domain descriptor; in2: domain descriptor invalid, no domain descriptor"));
}

// S1.POL.15
TEST_F(MultiReader2Policy, QueuedDomainFaultTakesEffectOnReadNotAvailableCount)
{
    setupBase();
    announce(in2, Domain().noOrigin());
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(avail(), count(100));
    expect(
        read(100),
        count(100).at(0).level(Error).inError(in2, InputError::DomainDescriptorInvalid).msg("in2: domain descriptor invalid, no origin"));
    expect(avail(), count(0));
}

// S1.POL.20
TEST_F(MultiReader2Policy, AcceptsDescriptorRejectAtConstructionInvalidatesReader)
{
    given().accepts().verdict(in2, Verdict::Reject);
    expect(read(0),
           count(0)
               .level(Error)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
}

// S1.POL.21
TEST_F(MultiReader2Policy, AcceptsDescriptorThrowAtConstructionCountsAsRejection)
{
    given().accepts().verdict(in2, Verdict::Throw);
    expect(read(0),
           count(0)
               .level(Error)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
}

// S1.POL.22
TEST_P(MultiReader2PolicyEachCall, ReadingInvalidatesWhenAcceptsDescriptorRejectsNewDescriptor)
{
    given().accepts();
    setupBase();
    verdict(in2, Verdict::Reject);
    announce(in2, Value(SampleType::Int32));
    expect(avail(), count(100).stubNotAsked());
    expect(reading(100),
           count(100)
               .at(0)
               .level(Error)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
}

// S1.POL.23
TEST_F(MultiReader2Policy, UnconvertibleValueTypeInvalidatesWithoutAskingAboutThatInput)
{
    given().accepts();
    setupBase();
    announce(in2, Value::structure());
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value type not convertible to Float64")
               .stubAsked({in1}));
}

// S1.POL.24
TEST_F(MultiReader2Policy, NonMainWithDifferentSamplePeriodInvalidatesReader)
{
    setupBase();
    announce(in2, Domain().rate500Hz());
    expect(read(100),
           count(100)
               .at(0)
               .level(Error)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, sampling rate differs from the main input's"));
}

// S1.POL.25
TEST_F(MultiReader2Policy, NonMainOffMainGridInvalidatesReaderAsSyncFailed)
{
    given().domain(in1, Domain().resolution(1, 2000).delta(2)).domain(in2, Domain().resolution(1, 2000).delta(2).start(1));
    write(in1, 2, 200);
    write(in2, 1, 199);
    expect(read(0), count(0).level(Error).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, off the main input's grid"));
}

// S1.POL.26
TEST_F(MultiReader2Policy, NonMainMoreThanTwoSecondsAheadInvalidatesAsSyncFailed)
{
    write(in1, 0, 99);
    write(in2, 2100, 2199);
    expect(read(0),
           count(0).level(Error).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, more than 2 s from the main input"));
}

// S1.POL.27
TEST_F(MultiReader2Policy, NonMainMoreThanTwoSecondsBehindInvalidatesAsSyncFailed)
{
    write(in2, 0, 99);
    write(in1, 2100, 2199);
    expect(read(0),
           count(0).level(Error).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, more than 2 s from the main input"));
}

// S1.POL.28
TEST_F(MultiReader2Policy, NonMainWithoutDataPastTwoSecondsInvalidatesAsSyncFailed)
{
    write(in1, 0, 2001);
    expect(read(0),
           count(0).level(Error).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, more than 2 s from the main input"));
}

// S1.POL.29
TEST_F(MultiReader2Policy, NonMainPortDisconnectInvalidatesReader)
{
    given().ports();
    setupBase();
    disconnect(in2);
    expect(read(100), count(100).at(0).level(Error).inError(in2, InputError::Disconnected).msg("in2: not connected"));
}

// S1.POL.31
TEST_F(MultiReader2Policy, NonMainSignalRemovedInvalidatesReaderAsDisconnected)
{
    setupBase();
    removeSignal(in2);
    expect(read(100), count(100).at(0).level(Error).inError(in2, InputError::Disconnected).msg("in2: not connected"));
}

// S1.POL.34
TEST_F(MultiReader2Policy, ExcludeKeepsAutomaticMainWhenNonMainDomainUnitNotSeconds)
{
    given().policy(Exclude);
    setupBase();
    announce(in2, Domain().unitMs());
    expect(read(100),
           count(100)
               .at(0)
               .level(Warning)
               .inError(in2, InputError::DomainDescriptorInvalid)
               .msg("in2: domain descriptor invalid, unit not seconds"));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.POL.37
TEST_F(MultiReader2Policy, ExcludeSetsAsideUnconnectedPortAtConstructionWithWarning)
{
    given().ports().policy(Exclude).unconnected(in2);
    write(in1, 0, 99);
    expect(
        read(0),
        count(0).level(Warning).domainChanged().resync().descChanged(in1).inError(in2, InputError::Disconnected).msg("in2: not connected"));
    expect(read(100), count(100).at(0).noChanges());
}

// S1.POL.38
TEST_F(MultiReader2Policy, ExcludeSetsAsideRejectedInputAtConstructionWithWarning)
{
    given().accepts().policy(Exclude).verdict(in2, Verdict::Reject);
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
}

// S1.POL.40
TEST_F(MultiReader2Policy, EqualConfigureSetsAsideNewlyRejectedInputUnderExclude)
{
    given().accepts().policy(Exclude);
    setupBase();
    verdict(in2, Verdict::Reject);
    expect(configure(same()),
           ok().level(Warning)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
    expect(read(100), count(100).at(0).noChanges());
}

// S1.POL.43
TEST_F(MultiReader2Policy, ExcludeSetsAsideOffGridNonMainWithWarning)
{
    given().policy(Exclude).domain(in1, Domain().resolution(1, 2000).delta(2)).domain(in2, Domain().resolution(1, 2000).delta(2).start(1));
    write(in1, 2, 200);
    write(in2, 1, 199);
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::SyncFailed)
               .msg("in2: not synchronized, off the main input's grid"));
}

// S1.POL.44
TEST_F(MultiReader2Policy, ExcludeSetsAsideNonMainMoreThanTwoSecondsAheadWithWarning)
{
    given().policy(Exclude);
    write(in1, 0, 99);
    write(in2, 2100, 2199);
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::SyncFailed)
               .msg("in2: not synchronized, more than 2 s from the main input"));
}

// S1.POL.47
TEST_F(MultiReader2Policy, ExcludeSetsAsideDisconnectedPortWithWarning)
{
    given().ports().policy(Exclude);
    setupBase();
    disconnect(in2);
    expect(read(100), count(100).at(0).level(Warning).inError(in2, InputError::Disconnected).msg("in2: not connected"));
}

// S1.POL.49
TEST_P(MultiReader2PolicyEachCall, ReadingAcceptsValueDescriptorChangeWithNullAcceptsDescriptor)
{
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(avail(), count(100));
    expect(reading(100), count(100).at(0).descChanged(in2));
}

// S1.POL.52
TEST_F(MultiReader2Policy, NullAcceptsDescriptorAcceptsAllInputsAtConstruction)
{
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
}

// S1.POL.53
TEST_P(MultiReader2PolicyEachCall, ReadingAsksAcceptsDescriptorForEveryInputOnNewValueDescriptor)
{
    given().accepts();
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(avail(), count(100).stubNotAsked());
    expect(reading(100), count(100).at(0).descChanged(in2).stubAsked({in1, in2}));
}

// S1.POL.55
TEST_F(MultiReader2Policy, EqualConfigureAsksAcceptsDescriptorForEveryInputAndKeepsData)
{
    given().accepts();
    setupBase();
    expect(configure(same()), ok().noChanges().stubAsked({in1, in2}));
    expect(read(100), count(100).at(0).noChanges());
}

// S1.POL.56
TEST_F(MultiReader2Policy, ConstructionAsksAcceptsDescriptorOnceForEveryInput)
{
    given().accepts();
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().stubAsked({in1, in2}));
}

// S1.POL.57
TEST_F(MultiReader2Policy, InvalidReaderAsksRejectingStubAgainOnDescriptorAndConfigure)
{
    given().accepts().verdict(in2, Verdict::Reject);
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(avail(), count(0).stubNotAsked());
    expect(read(0), count(0).noChanges().stubAsked({in1, in2}));
    expect(configure(same()), ok().noChanges().stubAsked({in1, in2}));
}

// S1.POL.58
TEST_F(MultiReader2Policy, ExcludeAsksRejectingStubAgainOnEachDescriptorChangeWhileSetAside)
{
    given().accepts().policy(Exclude).verdict(in2, Verdict::Reject);
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(skip(100), count(100).at(0).noChanges().stubAsked({in1, in2}));
    write(in1, 100, 199);
    announce(in2, Value::scaled(SampleType::Int16, 0.5, 1));
    expect(readWithDomain(100), count(100).at(100).noChanges().stubAsked({in1, in2}));
}

// S1.POL.59
TEST_F(MultiReader2Policy, InvalidReaderAsksThrowingStubAgainOnDescriptorAndConfigure)
{
    given().accepts().verdict(in2, Verdict::Throw);
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(avail(), count(0).stubNotAsked());
    expect(read(0), count(0).noChanges().stubAsked({in1, in2}));
    expect(configure(same()), ok().noChanges().stubAsked({in1, in2}));
}

// S1.POL.60
TEST_F(MultiReader2Policy, ExcludeAsksThrowingStubAgainOnEachDescriptorChangeWhileSetAside)
{
    given().accepts().policy(Exclude).verdict(in2, Verdict::Throw);
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(skip(100), count(100).at(0).noChanges().stubAsked({in1, in2}));
    write(in1, 100, 199);
    announce(in2, Value::scaled(SampleType::Int16, 0.5, 1));
    expect(readWithDomain(100), count(100).at(100).noChanges().stubAsked({in1, in2}));
}

// S1.POL.61
TEST_F(MultiReader2Policy, ReaderCallFromAcceptsDescriptorAtConstructionReturnsInvalidState)
{
    given().accepts();
    verdict(in2, Verdict::CallReader);
    announce(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expectStubSaw(OPENDAQ_ERR_INVALIDSTATE);
}

// S1.POL.62
TEST_P(MultiReader2PolicyEachCall, ReaderCallFromAcceptsDescriptorDuringReadReturnsInvalidState)
{
    given().accepts().verdict(in2, Verdict::CallReader);
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(avail(), count(100).stubNotAsked());
    expect(reading(100), count(100).at(0).descChanged(in2));
    expectStubSaw(OPENDAQ_ERR_INVALIDSTATE);
}

// S1.POL.64
TEST_F(MultiReader2Policy, ReaderCallFromAcceptsDescriptorDuringConfigureReturnsInvalidState)
{
    given().accepts().verdict(in2, Verdict::CallReader);
    setupBase();
    expect(configure(same()), ok().noChanges());
    expectStubSaw(OPENDAQ_ERR_INVALIDSTATE);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.POL.65
TEST_F(MultiReader2Policy, ExcludeSetsAsideInputRejectedDuringSkipSamplesWithWarning)
{
    given().accepts().policy(Exclude);
    setupBase();
    verdict(in2, Verdict::Reject);
    announce(in2, Value(SampleType::Int32));
    expect(skip(100),
           count(100)
               .at(0)
               .level(Warning)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
}

// S1.POL.67
TEST_F(MultiReader2Policy, ExcludeAcceptsNullBufferForSetAsideInput)
{
    given().ports().policy(Exclude).unconnected(in2);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(read(100, nullBuffer(in2)), count(100).at(0).noChanges());
}

// S1.POL.68
TEST_F(MultiReader2Policy, InvalidReaderAcceptsNullBufferForInputInError)
{
    given().ports().unconnected(in2);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(read(100, nullBuffer(in2)), count(0).noChanges());
}

// S1.POL.69
TEST_F(MultiReader2Policy, DescriptorQueuedBeforeFirstReadTakesEffectWhenReadReachesIt)
{
    write(in1, 0, 99);
    write(in2, 0, 49);
    announce(in2, Value(SampleType::Int32));
    write(in2, 50, 99);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(read(50), count(50).at(0).descChanged(in2));
    expect(read(50), count(50).at(50).noChanges());
}

// S1.POL.70
TEST_F(MultiReader2Policy, QueuedDescriptorJudgedByAcceptsDescriptorWhenReadReachesIt)
{
    given().accepts();
    write(in1, 0, 99);
    write(in2, 0, 49);
    announce(in2, Value(SampleType::Int32));
    write(in2, 50, 99);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().stubAsked({in1, in2}));
    expect(read(50), count(50).at(0).descChanged(in2).stubAsked({in1, in2}));
    expect(read(50), count(50).at(50).noChanges().stubNotAsked());
}

// S1.POL.71
TEST_F(MultiReader2Policy, RejectedAtConstructionStaysInErrorAndDropsDataOnNewDescriptor)
{
    given().accepts().verdict(in2, Verdict::Reject);
    expect(read(0),
           count(0)
               .level(Error)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
    write(in1, 0, 99);
    announce(in2, Value(SampleType::Int32));
    write(in2, 0, 99);
    expect(avail(), count(0).stubNotAsked());
    expect(read(0), count(0).noChanges().stubAsked({in1, in2}));
    expect(avail(), count(0));
}

// S1.POL.72
TEST_F(MultiReader2Policy, RejectionOfQueuedDescriptorInvalidatesReaderAndDropsQueuedData)
{
    given().accepts();
    write(in1, 0, 99);
    write(in2, 0, 49);
    announce(in2, Value(SampleType::Int32));
    write(in2, 50, 99);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().stubAsked({in1, in2}));
    verdict(in2, Verdict::Reject);
    expect(read(50),
           count(50)
               .at(0)
               .level(Error)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
    expect(avail(), count(0));
}

// S1.POL.73
TEST_F(MultiReader2Policy, ExcludeSetsAsideFaultyInputWhileSynchronizingAndStreamsMain)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    announce(in2, Domain().noOrigin());
    expect(avail(), count(0));
    expect(read(0),
           count(0).level(Warning).inError(in2, InputError::DomainDescriptorInvalid).msg("in2: domain descriptor invalid, no origin"));
    write(in1, 100, 199);
    expect(avail(), count(200));
    expect(read(200), count(200).at(0).noChanges());
}

// S1.POL.74
TEST_F(MultiReader2Policy, ReaderRecoversWhenNonMainDomainBecomesValidUnderInvalidate)
{
    given().domain(in2, Domain().noOrigin());
    setupBase();
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(avail(), count(0));
    expect(read(0), count(0).noChanges());
    announce(in2, Domain());
    write(in2, 200, 299);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    expect(avail(), count(0));
    write(in1, 300, 399);
    write(in2, 300, 399);
    expect(read(100), count(100).at(300).noChanges());
}

MR2_INSTANTIATE_EACH_READING_CALL(MultiReader2PolicyEachCall);

}
