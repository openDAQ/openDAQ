#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Recovery : public MultiReader2Test
{
};

// S1.REC.01
TEST_F(MultiReader2Recovery, DomainUnitErrorClearsOnValidDomainDescriptorUnderInvalidate)
{
    setupBase(
        [&]
        {
            announce(in2, Domain().unitMs());
            read(100);
        });
    write(in1, 100, 199);
    announce(in2, Domain().resolution(2, 2000));
    write(in2, 100, 199);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 200, 299);
    write(in2, 200, 299);
    expect(read(100), count(100).at(200).noChanges());
}

// S1.REC.02
TEST_F(MultiReader2Recovery, DeltaZeroInputRejoinsOnValidDomainDescriptorUnderExclude)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Domain().delta(0));
            read(100);
        });
    announce(in2, Domain().resolution(2, 2000));
    write(in2, 100, 199);
    write(in1, 100, 199);
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.03
TEST_F(MultiReader2Recovery, ValueDescriptorChangeDoesNotClearDomainDescriptorInvalid)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Domain().unitMs());
            read(100);
        });
    announce(in2, Value(SampleType::Int32));
    write(in2, 100, 199);
    write(in1, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.04
TEST_F(MultiReader2Recovery, PinnedMainRecoversFromNullDomainUnderExclude)
{
    given().policy(Exclude).main(in1);
    setupBase(
        [&]
        {
            announce(in1, Domain::null());
            read(100);
        });
    announce(in1, Domain().resolution(2, 2000));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().healthy(in1));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.05
TEST_F(MultiReader2Recovery, DifferentPeriodClearsOnCombinedValueAndDomainEvent)
{
    setupBase(
        [&]
        {
            announce(in2, Domain().rate500Hz());
            read(100);
        });
    announce(in2, Value(SampleType::Int32), Domain().resolution(2, 2000));
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.06
TEST_F(MultiReader2Recovery, GapDoesNotClearDifferentPeriodDomainDescriptorInvalid)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Domain().rate500Hz());
            read(100);
        });
    gap(in2);
    write(in1, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.07
TEST_F(MultiReader2Recovery, ReaderRecoversOnlyWhenLastDomainErrorClears)
{
    setupBase(
        [&]
        {
            announce(in1, Domain().unitMs());
            announce(in2, Domain().unitMs());
            read(100);
        });
    announce(in2, Domain().resolution(2, 2000));
    expect(read(0), count(0).healthy(in2));
    announce(in1, Domain().resolution(2, 2000));
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in1));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.08
TEST_F(MultiReader2Recovery, UnconnectedPortConnectRecoversInvalidReader)
{
    given().ports().unconnected(in2);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    connect(in2);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.09
TEST_F(MultiReader2Recovery, EqualConfigureDoesNotClearDisconnected)
{
    given().ports().unconnected(in2);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same()), ok().noChanges());
    write(in1, 100, 199);
    expect(avail(), count(0));
}

// S1.REC.10
TEST_F(MultiReader2Recovery, DisconnectedPortRejoinsOnConnectToOtherSignalUnderExclude)
{
    given().ports().policy(Exclude);
    setupBase(
        [&]
        {
            disconnect(in2);
            read(100);
        });
    connect(in2, s3);
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.11
TEST_F(MultiReader2Recovery, ConfigureRemovingDisconnectedInputRecoversReader)
{
    setupBase(
        [&]
        {
            removeSignal(in2);
            read(100);
        });
    expect(configure(same().remove(in2)), ok().level(Ok).main(in1).domainChanged().resync().allDescChanged().inputs({in1}));
    write(in1, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.12
TEST_F(MultiReader2Recovery, VerdictChangeToAcceptRecoversOnlyOnConfigure)
{
    given().accepts().verdict(in2, Verdict::Reject);
    setupBase();
    verdict(in2, Verdict::Accept);
    expect(read(0), count(0).noChanges());
    expect(configure(same()), ok().level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.13
TEST_F(MultiReader2Recovery, EqualValueDescriptorResentDoesNotAskAcceptsDescriptorAgain)
{
    given().accepts().policy(Exclude).verdict(in2, Verdict::Reject);
    setupBase([&] { read(100); });
    verdict(in2, Verdict::Accept);
    announce(in2, Value::same());
    write(in1, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.14
TEST_F(MultiReader2Recovery, RejectedInputRejoinsOnNewAcceptedDescriptorUnderExclude)
{
    given().accepts().policy(Exclude).verdict(in2, Verdict::Reject);
    setupBase([&] { read(100); });
    verdict(in2, Verdict::Accept);
    announce(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.15
TEST_F(MultiReader2Recovery, ThrowingVerdictInputRejoinsOnEqualConfigureAfterAccept)
{
    given().accepts().policy(Exclude).verdict(in2, Verdict::Throw);
    setupBase();
    verdict(in2, Verdict::Accept);
    expect(configure(same()), ok().level(Ok).descChanged(in2).healthy(in2));
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.REC.16
TEST_F(MultiReader2Recovery, RejectedInputRecoversOnNewAcceptedDescriptorUnderInvalidate)
{
    given().accepts();
    setupBase(
        [&]
        {
            verdict(in2, Verdict::Reject);
            configure(same());
        });
    verdict(in2, Verdict::Accept);
    announce(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.17
TEST_F(MultiReader2Recovery, UnconvertibleValueTypeClearsOnConvertibleValueDescriptor)
{
    setupBase(
        [&]
        {
            announce(in2, Value::structure());
            read(100);
        });
    announce(in2, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2).size(in1, 8).size(in2, 8));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.18
TEST_F(MultiReader2Recovery, ConfigureToUndefinedReadTypeClearsUnconvertibleValueType)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Value::structure());
            read(100);
        });
    expect(configure(same().readType(SampleType::Undefined)),
           ok().level(Ok).domainChanged().resync().allDescChanged().healthy(in2).size(in2, 12));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.19
TEST_F(MultiReader2Recovery, DomainChangeDoesNotClearUnconvertibleValueType)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Value::structure());
            read(100);
        });
    announce(in2, Domain().originSeconds(-1));
    write(in1, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.20
TEST_F(MultiReader2Recovery, MainMissingValueDescriptorRecoversOnNewValueDescriptor)
{
    setupBase(
        [&]
        {
            announce(in1, Value::null());
            read(100);
        });
    announce(in1, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in1));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.21
TEST_F(MultiReader2Recovery, EqualConfigureDoesNotClearMissingValueDescriptor)
{
    setupBase(
        [&]
        {
            announce(in2, Value::null());
            read(100);
        });
    expect(configure(same()), ok().noChanges());
    write(in1, 100, 199);
    expect(avail(), count(0));
}

// S1.REC.22
TEST_F(MultiReader2Recovery, OffGridSyncFailedClearsOnNewDomainDescriptor)
{
    given().domain(in1, Domain().delta(2)).domain(in2, Domain().delta(2));
    setup(
        [&]
        {
            read(0, nullData);
            write(in1, 0, 98);
            write(in2, 1, 99);
            read(0);
        });
    announce(in2, Domain().delta(2).originSeconds(-1));
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 100, 198);
    write(in2, 100, 198);
    expect(read(50), count(50).at(100).noChanges());
}

// S1.REC.23
TEST_F(MultiReader2Recovery, OffGridSyncFailedClearsOnGap)
{
    given().policy(Exclude).domain(in1, Domain().delta(2)).domain(in2, Domain().delta(2));
    setup(
        [&]
        {
            read(0, nullData);
            write(in1, 0, 98);
            write(in2, 1, 99);
            read(0);
            read(50);
        });
    gap(in2);
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    write(in1, 100, 198);
    write(in2, 100, 198);
    expect(read(50), count(50).at(100).noChanges());
}

// S1.REC.24
TEST_F(MultiReader2Recovery, OffGridSyncFailedClearsOnDataJumpAndResynchronizes)
{
    given().policy(Exclude).domain(in1, Domain().delta(2)).domain(in2, Domain().delta(2));
    setup(
        [&]
        {
            read(0, nullData);
            write(in1, 0, 98);
            write(in2, 1, 99);
            read(0);
            read(50);
        });
    write(in2, 200, 298);
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    write(in1, 100, 298);
    expect(read(50), count(0).resync());
    expect(read(50), count(50).at(200).noChanges());
}

// S1.REC.25
TEST_F(MultiReader2Recovery, OffGridSyncFailedClearsOnDisconnectAndConnect)
{
    given().ports().policy(Exclude).domain(in1, Domain().delta(2)).domain(in2, Domain().delta(2));
    setup(
        [&]
        {
            read(0, nullData);
            write(in1, 0, 98);
            write(in2, 1, 99);
            read(0);
            read(50);
            disconnect(in2);
        });
    expect(read(0), count(0).inError(in2, InputError::Disconnected).msg("in2: not connected"));
    connect(in2);
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2).msg(""));
    write(in1, 100, 198);
    write(in2, 100, 198);
    expect(read(50), count(50).at(100).noChanges());
}

// S1.REC.26
TEST_F(MultiReader2Recovery, OffGridSyncFailedClearsWhenMainInputChanges)
{
    given().policy(Exclude).main(in1).domain(in1, Domain().delta(2)).domain(in2, Domain().delta(2));
    setup(
        [&]
        {
            read(0, nullData);
            write(in1, 0, 98);
            write(in2, 1, 99);
            read(0);
            read(50);
        });
    expect(configure(same().main(in2)), ok().level(Ok).main(in2).domainChanged().resync().allDescChanged().healthy(in2));
    write(in2, 101, 199);
    write(in1, 101, 199);
    expect(read(50), count(50).at(101).noChanges());
}

// S1.REC.27
TEST_F(MultiReader2Recovery, ValueDescriptorChangeDoesNotClearOffGridSyncFailed)
{
    given().policy(Exclude).domain(in1, Domain().delta(2)).domain(in2, Domain().delta(2));
    setup(
        [&]
        {
            read(0, nullData);
            write(in1, 0, 98);
            write(in2, 1, 99);
            read(0);
            read(50);
        });
    announce(in2, Value(SampleType::Int32));
    write(in2, 101, 199);
    write(in1, 100, 198);
    expect(read(50), count(50).at(100).noChanges());
}

// S1.REC.28
TEST_F(MultiReader2Recovery, BehindSyncFailedClearsOnSampleWithinTwoSeconds)
{
    given().main(in1);
    setup(
        [&]
        {
            read(0, nullData);
            timeline(Timeline::Behind);
            read(0);
        });
    write(in2, 100, 199);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 2200, 2299);
    write(in2, 2200, 2299);
    expect(read(100), count(100).at(2200).noChanges());
}

// S1.REC.29
TEST_F(MultiReader2Recovery, EqualConfigureDoesNotClearBehindSyncFailed)
{
    given().main(in1);
    setup(
        [&]
        {
            read(0, nullData);
            timeline(Timeline::Behind);
            read(0);
        });
    write(in1, 2200, 2299);
    expect(read(0), count(0).noChanges());
    expect(configure(same()), ok().noChanges());
    expect(avail(), count(0));
}

// S1.REC.30
TEST_F(MultiReader2Recovery, ReinitializingConfigureClearsBehindSyncFailed)
{
    setup(
        [&]
        {
            read(0, nullData);
            timeline(Timeline::Behind);
            read(0);
        });
    expect(configure(same().minReadCount(5)), ok().level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 2200, 2299);
    write(in2, 2200, 2299);
    expect(read(100), count(100).at(2200).noChanges());
}

// S1.REC.31
TEST_F(MultiReader2Recovery, AheadSyncFailedClearsAndInputRejoinsWithResyncUnderExclude)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            read(0, nullData);
            timeline(Timeline::Ahead);
            read(0);
            read(100);
        });
    write(in1, 100, 199);
    expect(read(100), count(0).level(Ok).descChanged(in2).healthy(in2));
    write(in1, 200, 299);
    write(in2, 2200, 2299);
    write(in1, 300, 2299);
    expect(read(100), count(0).resync());
    expect(read(100), count(100).at(2200).noChanges());
}

// S1.REC.32
TEST_F(MultiReader2Recovery, AheadSyncFailedClearsWhenMainCatchesUpUnderInvalidate)
{
    setup(
        [&]
        {
            read(0, nullData);
            timeline(Timeline::Ahead);
            read(0);
        });
    write(in1, 100, 199);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 2200, 2299);
    write(in2, 2200, 2299);
    expect(read(100), count(100).at(2200).noChanges());
}

// S1.REC.33
TEST_F(MultiReader2Recovery, StarvedInputRejoinsWhenItsDataArrivesWithinTwoSeconds)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            read(0, nullData);
            timeline(Timeline::Starve2s);
            read(2002);
            read(2002);
        });
    write(in2, 3000, 3009);
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    write(in1, 3000, 3099);
    expect(avail(), count(0));
    write(in1, 3100, 3199);
    write(in2, 3100, 3199);
    expect(read(100), count(0).resync());
    expect(read(100), count(100).at(3100).noChanges());
}

// S1.REC.34
TEST_F(MultiReader2Recovery, StarvedSyncFailedSurvivesMainDomainChangeReconfigure)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            read(0, nullData);
            timeline(Timeline::Starve2s);
            read(2002);
            read(2002);
        });
    announce(in1, Domain().originSeconds(-1));
    expect(read(0), count(0).domainChanged().resync().allDescChanged());
    write(in1, 3000, 3099);
    expect(read(100), count(100).at(4000).noChanges());  // main ticks of the new origin, 1 s earlier
}

// S1.REC.35
TEST_F(MultiReader2Recovery, StarvedSyncFailedClearsWhenMainInputChanges)
{
    given().policy(Exclude).main(in1);
    setup(
        [&]
        {
            read(0, nullData);
            timeline(Timeline::Starve2s);
            read(2002);
            read(2002);
        });
    expect(configure(same().main(in2)), ok().level(Ok).main(in2).domainChanged().resync().allDescChanged().healthy(in2));
    write(in2, 3000, 3099);
    write(in1, 3000, 3099);
    expect(read(100), count(100).at(3000).noChanges());
}

// S1.REC.36
TEST_F(MultiReader2Recovery, HealedFormerMainRejoinsWithoutRetakingAutomaticMain)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in1, Value::structure());
            read(100);
            write(in2, 100, 199);
            read(100);
        });
    announce(in1, Value(SampleType::Int32));
    expect(read(0), count(0).level(Ok).main(in2).descChanged(in1).healthy(in1));
    write(in1, 200, 299);
    write(in2, 200, 299);
    expect(read(100), count(100).at(200).noChanges());
}

// S1.REC.37
TEST_F(MultiReader2Recovery, ReaderRecoversWithWarningWhenOneOfAllFailedInputsHeals)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in1, Value::structure());
            read(100);
            announce(in2, Value::structure());
            read(0);
        });
    announce(in1, Value(SampleType::Int32));
    expect(read(0), count(0).level(Warning).main(in1).domainChanged().resync().descChanged(in1).healthy(in1));
    write(in1, 200, 299);
    expect(read(100), count(100).at(200).noChanges());
}

// S1.REC.38
TEST_F(MultiReader2Recovery, OtherEventDoesNotClearMissingDomainDescriptor)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Domain::null());
            read(100);
        });
    otherEvent(in2);
    write(in1, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.39
TEST_F(MultiReader2Recovery, DisconnectReplacesDomainErrorAndConnectToValidSignalRecovers)
{
    given().ports().signal(in2, sig::domainFault(7));
    setupBase([&] { disconnect(in2); });
    expect(read(0), count(0).inError(in2, InputError::Disconnected).msg("in2: not connected"));
    connect(in2, s3);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2).msg(""));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.REC.40
TEST_F(MultiReader2Recovery, AutomaticMainMoveReevaluatesSyncFailedInputAfresh)
{
    given().inputs(10).policy(Exclude);
    setup(
        [&]
        {
            write(in1, 0, 99);
            write(in3, 0, 99);
            write(in4, 0, 99);
            write(in5, 0, 99);
            write(in6, 0, 99);
            write(in7, 0, 99);
            write(in8, 0, 99);
            write(in9, 0, 99);
            write(in10, 0, 99);
            write(in2, 2100, 2199);
            read(0, nullData);
        });
    announce(in1, Value::null());
    expect(read(100),
           count(100)
               .at(0)
               .main(in3)
               .domainChanged()
               .resync()
               .descChanged(in2)
               .descChanged(in3)
               .descChanged(in4)
               .descChanged(in5)
               .descChanged(in6)
               .descChanged(in7)
               .descChanged(in8)
               .descChanged(in9)
               .descChanged(in10)
               .inError(in1, InputError::ValueDescriptorInvalid)
               .healthy(in2));
    write(in3, 2200, 2299);
    write(in4, 2200, 2299);
    write(in5, 2200, 2299);
    write(in6, 2200, 2299);
    write(in7, 2200, 2299);
    write(in8, 2200, 2299);
    write(in9, 2200, 2299);
    write(in10, 2200, 2299);
    expect(avail(), count(0));
    write(in2, 2200, 2299);
    expect(read(100), count(100).at(2200).noChanges());
}

// S1.REC.41
TEST_F(MultiReader2Recovery, MissingValueDescriptorInputRejoinsWhenDescriptorAssigned)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            write(in1, 0, 99);
            announce(in2, Value::null());
            write(in2, 0, 99);
            read(0, nullData);
        });
    expect(read(100), count(100).at(0).noChanges());
    write(in1, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
    announce(in2, Value());
    write(in2, 100, 199);
    write(in1, 200, 299);
    write(in2, 200, 299);
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    expect(read(100), count(100).at(200).noChanges());
}

// S1.REC.42
TEST_F(MultiReader2Recovery, ConnectToOtherSignalAsksAcceptsDescriptorAndReportsChange)
{
    given().ports().accepts().value(s3, Value(SampleType::Int32));
    setupBase();
    connect(in2, s3);
    expect(read(300), count(100).at(0).descChanged(in2).stubAsked({in1, in2}));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(300), count(100).at(100).noChanges());
}

// S1.REC.43
TEST_F(MultiReader2Recovery, ConnectToRejectedSignalInvalidatesReader)
{
    given().ports().accepts().value(s3, Value(SampleType::Int32));
    setupBase();
    verdict(in2, Verdict::Reject);
    connect(in2, s3);
    expect(read(300),
           count(100)
               .at(0)
               .level(Error)
               .inError(in2, InputError::ValueDescriptorInvalid)
               .msg("in2: value descriptor rejected by the owner")
               .stubAsked({in1, in2}));
}

}
