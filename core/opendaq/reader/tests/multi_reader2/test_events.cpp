#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Events : public MultiReader2Test
{
};

// S1.EVT.02
TEST_F(MultiReader2Events, MainValueDescriptorChangeBeforeDataReadsZeroWithChange)
{
    setupBase([&] { read(100); });
    announce(in1, Value(SampleType::Int32));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(0).descChanged(in1));
    expect(read(100), count(100).at(100).noChanges());
}

// S1.EVT.03
TEST_F(MultiReader2Events, NonMainValueDescriptorChangeStopsReadAndReportsWithData)
{
    setupBase();
    announce(in2, Value(SampleType::Int32));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(300), count(100).at(0).descChanged(in2));
    expect(read(300), count(100).at(100).noChanges());
}

// S1.EVT.04
TEST_F(MultiReader2Events, ReadEndingExactlyAtValueDescriptorChangeReportsIt)
{
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(read(100), count(100).at(0).descChanged(in2));
}

// S1.EVT.05
TEST_F(MultiReader2Events, ReadShortOfValueDescriptorChangeDoesNotReportIt)
{
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(read(50), count(50).at(0).noChanges());
    expect(read(100), count(50).at(50).descChanged(in2));
}

// S1.EVT.06
TEST_F(MultiReader2Events, SuccessiveValueDescriptorChangesEachStopOneRead)
{
    setupBase();
    announce(in2, Value(SampleType::Int32));
    write(in1, 100, 199);
    write(in2, 100, 199);
    announce(in2, Value().dims({3}));
    expect(read(300), count(100).at(0).descChanged(in2));
    expect(read(300), count(100).at(100).descChanged(in2));
}

// S1.EVT.07
TEST_F(MultiReader2Events, ValueDescriptorChangeBelowMinReadCountDropsSamplesAndResynchronizes)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    announce(in2, Value(SampleType::Int32));
    write(in1, 109, 208);
    write(in2, 109, 208);
    expect(read(100), count(0).resync().descChanged(in2));
    expect(read(100), count(100).at(109).noChanges());
}

// S1.EVT.08
TEST_F(MultiReader2Events, ValueDescriptorChangeOnExcludedInputIsNotReported)
{
    given().policy(Exclude).domain(in2, Domain().unitMs());
    setupBase();
    announce(in2, Value(SampleType::Int32));
    expect(read(200), count(100).at(0).noChanges());
}

// S1.EVT.09
TEST_F(MultiReader2Events, MainDomainChangeBeforeDataReportsDomainChangedAndResync)
{
    setupBase([&] { read(100); });
    announce(in1, Domain().start(3));
    expect(read(100), count(0).domainChanged().resync().allDescChanged());
}

// S1.EVT.10
TEST_F(MultiReader2Events, MainDomainChangeBehindDataReportsWithPrecedingSamples)
{
    setupBase();
    announce(in1, Domain().start(3));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(300), count(100).at(0).domainChanged().resync().allDescChanged());
    write(in1, 200, 299);
    write(in2, 200, 299);
    expect(read(300), count(100).at(200).noChanges());
}

// S1.EVT.11
TEST_F(MultiReader2Events, NonMainDomainChangeResynchronizesWithoutDomainChanged)
{
    setupBase();
    announce(in2, Domain().start(3));
    expect(read(300), count(100).at(0).noChanges());
    write(in1, 100, 199);
    write(in2, 150, 249);
    expect(read(100), count(0).resync());
    expect(read(100), count(50).at(150).noChanges());
}

// S1.EVT.12
TEST_F(MultiReader2Events, NonMainDomainChangeBelowMinReadCountDropsSamplesAndResynchronizes)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    announce(in2, Domain().start(3));
    expect(read(100), count(0).resync());
    write(in1, 109, 208);
    write(in2, 109, 208);
    expect(read(100), count(100).at(109).noChanges());
}

// S1.EVT.13
TEST_F(MultiReader2Events, NonMainValueAndDomainEventBeforeDataReportsOnlyDescriptorChanged)
{
    setupBase([&] { read(100); });
    announce(in2, Value(SampleType::Int32), Domain().start(3));
    expect(read(100), count(0).descChanged(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.EVT.14
TEST_F(MultiReader2Events, NonMainValueAndDomainEventBehindDataReportsWithPrecedingSamples)
{
    setupBase();
    announce(in2, Value(SampleType::Int32), Domain().start(3));
    expect(read(300), count(100).at(0).descChanged(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.EVT.15
TEST_F(MultiReader2Events, MainValueAndDomainEventBelowMinReadCountReportsDomainChanged)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    announce(in1, Value(SampleType::Int32), Domain().start(3));
    expect(read(100), count(0).domainChanged().resync().allDescChanged());
}

// S1.EVT.16
TEST_F(MultiReader2Events, GapOnMainInputBeforeDataResynchronizesReader)
{
    setupBase([&] { read(100); });
    gap(in1);
    expect(read(100), count(0).noChanges());
    write(in1, 150, 249);
    write(in2, 100, 199);
    expect(read(100), count(0).resync());
    expect(read(100), count(50).at(150).noChanges());
}

// S1.EVT.17
TEST_F(MultiReader2Events, GapOnNonMainInputBehindDataResynchronizesReader)
{
    setupBase();
    gap(in2);
    expect(read(300), count(100).at(0).noChanges());
    write(in1, 100, 199);
    write(in2, 150, 249);
    expect(read(100), count(0).resync());
    expect(read(100), count(50).at(150).noChanges());
}

// S1.EVT.18
TEST_F(MultiReader2Events, GapOnMainInputBelowMinReadCountDropsSamplesAndResynchronizes)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    gap(in1);
    expect(read(100), count(0).resync());
    write(in1, 109, 208);
    write(in2, 109, 208);
    expect(read(100), count(100).at(109).noChanges());
}

// S1.EVT.19
TEST_F(MultiReader2Events, ConnectingOtherSignalToMainPortReportsDescriptorChanged)
{
    given().ports().value(s3, Value(SampleType::Int32));
    setupBase([&] { read(100); });
    connect(in1, s3);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(0).descChanged(in1));
    expect(read(100), count(100).at(100).noChanges());
}

// S1.EVT.20
TEST_F(MultiReader2Events, ConnectingOtherSignalToNonMainPortBehindDataReportsWithSamples)
{
    given().ports().value(s3, Value(SampleType::Int32));
    setupBase();
    connect(in2, s3);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(300), count(100).at(0).descChanged(in2));
    expect(read(300), count(100).at(100).noChanges());
}

// S1.EVT.21
TEST_F(MultiReader2Events, ConnectingOtherSignalBelowMinReadCountDropsSamplesAndResynchronizes)
{
    given().ports().minReadCount(10).value(s3, Value(SampleType::Int32));
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    connect(in2, s3);
    write(in1, 109, 208);
    write(in2, 109, 208);
    expect(read(100), count(0).resync().descChanged(in2));
    expect(read(100), count(100).at(109).noChanges());
}

// S1.EVT.22
TEST_F(MultiReader2Events, DisconnectingMainPortBeforeDataReportsDisconnectedError)
{
    given().ports();
    setupBase([&] { read(100); });
    disconnect(in1);
    expect(read(100), count(0).level(Error).inError(in1, InputError::Disconnected).msg("in1: not connected"));
}

// S1.EVT.23
TEST_F(MultiReader2Events, DisconnectingNonMainPortBehindDataReportsErrorWithSamples)
{
    given().ports();
    setupBase();
    disconnect(in2);
    expect(read(300), count(100).at(0).level(Error).inError(in2, InputError::Disconnected).msg("in2: not connected"));
}

// S1.EVT.24
TEST_F(MultiReader2Events, DisconnectingPortBelowMinReadCountReportsErrorWithZeroCount)
{
    given().ports().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    disconnect(in2);
    expect(read(100), count(0).level(Error).inError(in2, InputError::Disconnected));
}

// S1.EVT.25
TEST_F(MultiReader2Events, MainPortDisconnectAndReconnectBeforeDataIsNotReported)
{
    given().ports();
    setupBase([&] { read(100); });
    disconnect(in1);
    connect(in1);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(200), count(100).at(100).noChanges());
}

// S1.EVT.26
TEST_F(MultiReader2Events, NonMainPortDisconnectAndReconnectBehindDataDoesNotStopRead)
{
    given().ports();
    setupBase();
    disconnect(in2);
    connect(in2);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(300), count(200).at(0).noChanges());
}

// S1.EVT.27
TEST_F(MultiReader2Events, DisconnectAndReconnectBelowMinReadCountKeepsSamples)
{
    given().ports().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    disconnect(in2);
    connect(in2);
    write(in1, 109, 208);
    write(in2, 109, 208);
    expect(read(200), count(109).at(100).noChanges());
}

// S1.EVT.28
TEST_F(MultiReader2Events, EqualValueDescriptorResentBeforeDataIsNotReported)
{
    setupBase([&] { read(100); });
    announce(in2, Value::same());
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(200), count(100).at(100).noChanges());
}

// S1.EVT.29
TEST_F(MultiReader2Events, EqualDomainDescriptorResentBehindDataDoesNotStopRead)
{
    setupBase();
    announce(in1, Domain::same());
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(200), count(200).at(0).noChanges());
}

// S1.EVT.30
TEST_F(MultiReader2Events, EqualDescriptorResentBelowMinReadCountKeepsSamples)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    announce(in2, Value::same());
    write(in1, 109, 208);
    write(in2, 109, 208);
    expect(read(200), count(109).at(100).noChanges());
}

// S1.EVT.31
TEST_F(MultiReader2Events, NullValueDescriptorOnMainBeforeDataReportsError)
{
    setupBase([&] { read(100); });
    announce(in1, Value::null());
    expect(read(100), count(0).level(Error).msg("in1: no value descriptor"));
}

// S1.EVT.32
TEST_F(MultiReader2Events, NullValueDescriptorOnNonMainBehindDataReportsErrorWithSamples)
{
    setupBase();
    announce(in2, Value::null());
    expect(read(300), count(100).at(0).level(Error).msg("in2: no value descriptor"));
}

// S1.EVT.33
TEST_F(MultiReader2Events, NullValueDescriptorBelowMinReadCountReportsErrorWithZeroCount)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    announce(in1, Value::null());
    expect(read(100), count(0).level(Error));
}

// S1.EVT.34
TEST_F(MultiReader2Events, NullDomainDescriptorOnNonMainBeforeDataReportsError)
{
    setupBase([&] { read(100); });
    announce(in2, Domain::null());
    expect(read(100), count(0).level(Error).msg("in2: domain descriptor invalid, no domain descriptor"));
}

// S1.EVT.35
TEST_F(MultiReader2Events, NullDomainDescriptorOnMainBehindDataReportsErrorWithSamples)
{
    setupBase();
    announce(in1, Domain::null());
    expect(read(300), count(100).at(0).level(Error).msg("in1: domain descriptor invalid, no domain descriptor"));
}

// S1.EVT.36
TEST_F(MultiReader2Events, NullDomainDescriptorBelowMinReadCountReportsErrorWithZeroCount)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    announce(in2, Domain::null());
    expect(read(100), count(0).level(Error));
}

// S1.EVT.37
TEST_F(MultiReader2Events, RemovingMainSignalBeforeDataReportsDisconnectedError)
{
    setupBase([&] { read(100); });
    removeSignal(in1);
    expect(read(100), count(0).level(Error).inError(in1, InputError::Disconnected).msg("in1: not connected"));
}

// S1.EVT.38
TEST_F(MultiReader2Events, RemovingNonMainSignalBehindDataReportsErrorWithSamples)
{
    setupBase();
    removeSignal(in2);
    expect(read(300), count(100).at(0).level(Error).inError(in2, InputError::Disconnected).msg("in2: not connected"));
}

// S1.EVT.39
TEST_F(MultiReader2Events, RemovingSignalBelowMinReadCountReportsErrorWithZeroCount)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    removeSignal(in1);
    expect(read(100), count(0).level(Error));
}

// S1.EVT.40
TEST_F(MultiReader2Events, OtherEventOnNonMainInputIsIgnored)
{
    setupBase();
    otherEvent(in2);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(200), count(200).at(0).noChanges());
}

// S1.EVT.41
TEST_F(MultiReader2Events, OtherEventOnMainInputIsIgnored)
{
    setupBase();
    otherEvent(in1);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(200), count(200).at(0).noChanges());
}

// S1.EVT.42
TEST_F(MultiReader2Events, ConnectFollowedByEqualValueDescriptorReportsOneChange)
{
    given().ports().value(s3, Value(SampleType::Int32));
    setupBase();
    connect(in2, s3);
    announce(in2, Value::same());
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(300), count(100).at(0).descChanged(in2));
    expect(read(300), count(100).at(100).noChanges());
}

// S1.EVT.43
TEST_F(MultiReader2Events, DisconnectThenConnectOfOtherSignalReportsOnlyDescriptorChanged)
{
    given().ports().value(s3, Value(SampleType::Int32));
    setupBase();
    disconnect(in1);
    connect(in1, s3);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(300), count(100).at(0).descChanged(in1));
    expect(read(300), count(100).at(100).noChanges());
}

// S1.EVT.44
TEST_F(MultiReader2Events, ReconnectThenValueDescriptorChangeReportsOnlyDescriptorChanged)
{
    given().ports();
    setupBase();
    disconnect(in2);
    connect(in2);
    announce(in2, Value(SampleType::Int32));
    expect(read(300), count(100).at(0).descChanged(in2));
}

// S1.EVT.45
TEST_F(MultiReader2Events, ConsecutiveNonMainValueAndDomainEventsAreReportedOnce)
{
    setupBase();
    announce(in2, Value(SampleType::Int32));
    announce(in2, Domain().start(3));
    expect(read(300), count(100).at(0).descChanged(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.EVT.46
TEST_F(MultiReader2Events, ConsecutiveMainValueAndDomainEventsAreReportedOnce)
{
    setupBase();
    announce(in1, Value(SampleType::Int32));
    announce(in1, Domain().start(3));
    expect(read(300), count(100).at(0).domainChanged().resync().allDescChanged());
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.EVT.47
TEST_F(MultiReader2Events, NullValueDescriptorFollowedByValidOneClearsError)
{
    setupBase();
    announce(in2, Value::null());
    announce(in2, Value(SampleType::Int32));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(300), count(100).at(0).descChanged(in2));
    expect(read(300), count(100).at(100).noChanges());
}

// S1.EVT.48
TEST_F(MultiReader2Events, DescriptorChangeFollowedByNullDomainEndsInError)
{
    setupBase();
    announce(in2, Value(SampleType::Int32), Domain().start(3));
    announce(in2, Domain::null());
    expect(read(300), count(100).at(0).level(Error));
}

// S1.EVT.49
TEST_F(MultiReader2Events, GapFollowedBySignalRemovalEndsInError)
{
    setupBase();
    gap(in1);
    removeSignal(in1);
    expect(read(300), count(100).at(0).level(Error));
}

// S1.EVT.50
TEST_F(MultiReader2Events, NonMainJumpAheadMidStreamResynchronizesAfterQueuedData)
{
    setupBase();
    write(in1, 100, 599);
    write(in2, 500, 599);
    expect(read(300), count(100).at(0).resync());
    expect(read(300), count(100).at(500).noChanges());
}

// S1.EVT.51
TEST_F(MultiReader2Events, NonMainJumpBackBelowMinReadCountDropsSamplesAndResynchronizes)
{
    given().minReadCount(10);
    setupBase(
        [&]
        {
            read(100);
            write(in1, 100, 108);
            write(in2, 100, 108);
        });
    write(in1, 109, 208);
    write(in2, 50, 208);
    expect(read(100), count(0).resync());
    expect(read(100), count(100).at(109).noChanges());
}

}
