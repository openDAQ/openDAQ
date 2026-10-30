#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Read : public MultiReader2Test
{
};

class MultiReader2ReadEachCall : public EachReadingCall
{
};

// S1.READ.01
TEST_P(MultiReader2ReadEachCall, ReadingAvailableSamplesLeavesNoneAvailable)
{
    setupBase();
    expect(reading(100), count(100).at(0).noChanges());
    expect(avail(), count(0));
}

// S1.READ.02
TEST_P(MultiReader2ReadEachCall, ReadingMoreThanAvailableReturnsRemainderWithoutWaiting)
{
    setupBase();
    expect(reading(40), count(40).at(0).noChanges());
    expect(reading(100), count(60).at(40).noChanges());
}

// S1.READ.03
TEST_P(MultiReader2ReadEachCall, ReadingZeroCountIsStatusReadAndKeepsData)
{
    setupBase();
    expect(reading(0), count(0).noChanges());
    expect(read(0, nullData), count(0).noChanges());
    expect(avail(), count(100));
}

// S1.READ.04
TEST_P(MultiReader2ReadEachCall, ReadingBelowMinReadCountFailsWithInvalidParameter)
{
    given().minReadCount(10);
    setupBase();
    expect([&] { return reading(5); }, fails(OPENDAQ_ERR_INVALIDPARAMETER).unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.READ.13
TEST_F(MultiReader2Read, ReadAtMinReadCountReturnsThatManySamples)
{
    given().minReadCount(10);
    setupBase();
    expect(read(10), count(10).at(0).noChanges());
    expect(avail(), count(90));
}

// S1.READ.14
TEST_F(MultiReader2Read, ReadKeepsFewerThanMinReadCountSamplesAtOpenEnd)
{
    given().minReadCount(10);
    setup(
        [&]
        {
            write(in1, 0, 4);
            write(in2, 0, 4);
            read(0, nullData);
        });
    expect(avail(), count(0));
    expect(read(10), count(0).noChanges());
    write(in1, 5, 9);
    write(in2, 5, 9);
    expect(read(10), count(10).at(0).noChanges());
}

// S1.READ.15
TEST_F(MultiReader2Read, AvailableCountAddsUpSingleSamplePackets)
{
    given().onePerPacket();
    setup(
        [&]
        {
            write(in1, 0, 0);
            write(in2, 0, 0);
            read(0, nullData);
        });
    write(in1, 1, 1);
    write(in2, 1, 1);
    expect(avail(), count(2));
    expect(read(2), count(2).at(0).noChanges());
}

// S1.READ.16
TEST_F(MultiReader2Read, ReadStopsWhereShortestInputRunsOut)
{
    given().differingPackets();
    setupBase();
    write(in1, 100, 199);
    write(in2, 100, 149);
    expect(avail(), count(150));
    expect(read(200), count(150).at(0).noChanges());
    write(in2, 150, 199);
    expect(read(200), count(50).at(150).noChanges());
}

// S1.READ.17
TEST_F(MultiReader2Read, EmptyDataPacketIsIgnored)
{
    setupBase();
    writeEmpty(in2);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(200), count(200).at(0).noChanges());
}

// S1.READ.18
TEST_F(MultiReader2Read, AvailableCountIsZeroUntilFirstStatusIsRead)
{
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(avail(), count(0));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(avail(), count(100));
}

// S1.READ.19
TEST_P(MultiReader2ReadEachCall, FirstReadingCallReturnsZeroWithFirstStatus)
{
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(reading(100), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(reading(100), count(100).at(0).noChanges());
}

// S1.READ.23
TEST_F(MultiReader2Read, EveryReadCallReturnsZeroWhileSynchronizing)
{
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(avail(), count(0));
    expect(read(100), count(0).noChanges());
    expect(skip(100), count(0).noChanges());
    expect(readWithDomain(100), count(0).noChanges());
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.READ.25
TEST_F(MultiReader2Read, AvailableCountReportsAlignedSamplesWhenSynced)
{
    setupBase();
    expect(avail(), count(100));
    expect(read(100), count(100).at(0).noChanges());
}

// S1.READ.26
TEST_F(MultiReader2Read, InputStatusForUnknownInputFailsWithInvalidParameter)
{
    setupBase();
    expect([&] { return inputStatus(in9); }, fails(OPENDAQ_ERR_INVALIDPARAMETER).unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.READ.27
TEST_F(MultiReader2Read, AvailableCountIgnoresExcludedInputUnderWarning)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Domain().unitMs());
            read(200);
        });
    write(in1, 100, 199);
    expect(avail(), count(100));
}

// S1.READ.28
TEST_P(MultiReader2ReadEachCall, ReadingUnderWarningLeavesExcludedInputBufferUntouched)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Domain().unitMs());
            read(200);
        });
    write(in1, 100, 199);
    expect(reading(100), count(100).at(100).noChanges());
    expect(avail(), count(0));
}

// S1.READ.32
TEST_F(MultiReader2Read, ReadAcceptsNullBufferForExcludedInput)
{
    given().policy(Exclude);
    setupBase(
        [&]
        {
            announce(in2, Domain().unitMs());
            read(200);
        });
    write(in1, 100, 199);
    expect(read(100, nullBuffer(in2)), count(100).at(100).noChanges());
}

// S1.READ.33
TEST_F(MultiReader2Read, AvailableCountAndEveryReadCallAreZeroUnderError)
{
    setupBase(
        [&]
        {
            announce(in2, Domain().unitMs());
            read(200);
        });
    write(in1, 100, 199);
    expect(avail(), count(0));
    expect(read(100), count(0).noChanges());
    expect(skip(100), count(0).noChanges());
    expect(readWithDomain(100), count(0).noChanges());
}

// S1.READ.35
TEST_P(MultiReader2ReadEachCall, ReadingWithNullBufferForHealthyInputFailsWithInvalidParameter)
{
    MR2_SKIP_CALL_WITHOUT_BUFFERS();
    setupBase();
    expect([&] { return reading(100, nullBuffer(in2)); }, fails(OPENDAQ_ERR_INVALIDPARAMETER).unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.READ.37
TEST_P(MultiReader2ReadEachCall, ReadingWithNullCountOrStatusFailsWithArgumentNull)
{
    setupBase();
    expect([&] { return reading(100, nullCount); }, fails(OPENDAQ_ERR_ARGUMENT_NULL).unchanged());
    expect([&] { return reading(100, nullStatus); }, fails(OPENDAQ_ERR_ARGUMENT_NULL).unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.READ.40
TEST_P(MultiReader2ReadEachCall, ReadingWithNonZeroTimeoutFailsWithNotImplemented)
{
    setupBase();
    expect([&] { return reading(100, timeout(10)); }, fails(OPENDAQ_ERR_NOTIMPLEMENTED).unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.READ.43
TEST_F(MultiReader2Read, AvailableCountIsZeroWhileDescriptorChangeIsOwed)
{
    setupBase([&] { read(100); });
    announce(in2, Value().dims({3}));
    write(in1, 100, 199);
    expect(avail(), count(0));
    expect(read(100), count(0).descChanged(in2));
}

// S1.READ.44
TEST_F(MultiReader2Read, StatusReadReportsDescriptorChangeBeforeItsData)
{
    setupBase([&] { read(100); });
    announce(in2, Value().dims({3}));
    write(in1, 100, 199);
    expect(read(0), count(0).descChanged(in2));
    write(in2, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.READ.45
TEST_F(MultiReader2Read, ReadInPartsStopsAtQueuedDescriptorChange)
{
    setupBase();
    announce(in2, Value().dims({3}));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(avail(), count(100));
    expect(read(0), count(0).noChanges());
    expect(read(40), count(40).at(0).noChanges());
    expect(read(100), count(60).at(40).descChanged(in2));
    expect(read(100), count(100).at(100).noChanges());
}

// S1.READ.46
TEST_F(MultiReader2Read, ReadReachingQueuedDescriptorChangeReportsIt)
{
    setupBase();
    announce(in2, Value().dims({3}));
    write(in1, 100, 199);
    expect(read(100), count(100).at(0).descChanged(in2));
}

// S1.READ.47
TEST_F(MultiReader2Read, ReadInPartsReportsConsecutiveDescriptorChangesOnce)
{
    setupBase();
    announce(in2, Value(SampleType::Int32));
    announce(in2, Value().dims({3}));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(read(0), count(0).noChanges());
    expect(read(40), count(40).at(0).noChanges());
    expect(read(100), count(60).at(40).descChanged(in2));
    expect(read(100), count(100).at(100).noChanges());
}

// S1.READ.48
TEST_F(MultiReader2Read, ReadReachingConsecutiveDescriptorChangesReportsThemOnce)
{
    setupBase();
    announce(in2, Value(SampleType::Int32));
    announce(in2, Value().dims({3}));
    write(in1, 100, 199);
    expect(read(100), count(100).at(0).descChanged(in2));
}

// S1.READ.58
TEST_P(MultiReader2ReadEachCall, ReadingDropsFewerThanMinReadCountSamplesBeforeEvent)
{
    given().minReadCount(10);
    setup(
        [&]
        {
            write(in1, 0, 18);
            write(in2, 0, 18);
            read(0, nullData);
            read(10);
        });
    announce(in2, Value().dims({3}));
    write(in1, 19, 118);
    write(in2, 19, 118);
    expect(avail(), count(0));
    expect(reading(10), count(0).resync().descChanged(in2));
    expect(read(100), count(100).at(19).noChanges());
}

// S1.READ.59
TEST_F(MultiReader2Read, StatusReadDropsFewerThanMinReadCountSamplesBeforeEvent)
{
    given().minReadCount(10);
    setup(
        [&]
        {
            write(in1, 0, 18);
            write(in2, 0, 18);
            read(0, nullData);
            read(10);
        });
    announce(in2, Value().dims({3}));
    write(in1, 19, 118);
    write(in2, 19, 118);
    expect(read(0), count(0).resync().descChanged(in2));
    expect(read(100), count(100).at(19).noChanges());
}

// S1.READ.62
TEST_F(MultiReader2Read, AvailableCountIsZeroUntilMissingNonMainInputDelivers)
{
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(avail(), count(0));
    write(in1, 100, 199);
    expect(avail(), count(0));
    write(in2, 0, 99);
    expect(avail(), count(100));
}

// S1.READ.63
TEST_F(MultiReader2Read, AvailableCountIsZeroUntilAllTenInputsDeliver)
{
    given().inputs(10);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(avail(), count(0));
    write(in2, 0, 99);
    write(in3, 0, 99);
    write(in4, 0, 99);
    write(in5, 0, 99);
    write(in6, 0, 99);
    write(in7, 0, 99);
    write(in8, 0, 99);
    write(in9, 0, 99);
    expect(avail(), count(0));
    write(in10, 0, 99);
    expect(avail(), count(100));
}

// S1.READ.64
TEST_F(MultiReader2Read, AvailableCountIsZeroUntilMainInputDelivers)
{
    setup(
        [&]
        {
            write(in2, 0, 99);
            read(0, nullData);
        });
    expect(avail(), count(0));
    write(in1, 0, 99);
    expect(avail(), count(100));
}

// S1.READ.65
TEST_F(MultiReader2Read, AvailableCountReachesMinReadCountOnlyWhenEveryInputHasIt)
{
    given().minReadCount(10);
    setup(
        [&]
        {
            write(in1, 0, 8);
            write(in2, 0, 8);
            read(0, nullData);
        });
    expect(avail(), count(0));
    write(in1, 9, 9);
    expect(avail(), count(0));
    write(in2, 9, 9);
    expect(avail(), count(10));
}

// S1.READ.66
TEST_F(MultiReader2Read, RecoveryFromLateInputDropsDataQueuedBeforeIt)
{
    setup(
        [&]
        {
            write(in1, 0, 2001);
            read(0, nullData);
        });
    expect(avail(), count(0));
    write(in2, 2000, 2099);
    expect(avail(), count(0));
    expect(read(0, nullData), count(0).level(Ok).domainChanged().resync().allDescChanged().healthy(in2));
    expect(avail(), count(0));
    write(in1, 2100, 2199);
    write(in2, 2100, 2199);
    expect(avail(), count(100));
}

// S1.READ.67
TEST_F(MultiReader2Read, DisconnectAndReconnectWithNothingBetweenAreNotReported)
{
    given().ports();
    setupBase();
    disconnect(in2);
    connect(in2);
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(avail(), count(200));
    expect(read(200), count(200).at(0).noChanges());
}

MR2_INSTANTIATE_EACH_READING_CALL(MultiReader2ReadEachCall);

}
