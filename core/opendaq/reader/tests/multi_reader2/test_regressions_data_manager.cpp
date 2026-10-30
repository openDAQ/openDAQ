#include "multi_reader2_harness.h"

namespace daq::mr2test
{

// Regressions of the data manager review (2026-10-29); each test names the finding it covers
class MultiReader2RegressionDm : public MultiReader2Test
{
};

// Finding 1
TEST_F(MultiReader2RegressionDm, SyncDroppingDataInFrontOfAnEventCallsTheHandler)
{
    given().main(in1);
    setup([&] { read(0, nullData); });
    handler([&] { drain(); });
    write(in2, 0, 99);
    announce(in2, Value().unit("A"));
    expect(idle(), calls(0));
    write(in1, 500, 599);
    expect(idle(), calls(1, {count(0).descChanged(in2), count(0).noChanges()}));
    write(in2, 500, 599);
    expect(idle(), calls(1, {count(100).at(500).noChanges(), count(0).noChanges()}));
}

// Finding 2
TEST_F(MultiReader2RegressionDm, MainJumpingAheadWhileStreamingFailsNoOtherInput)
{
    setupBase();
    write(in1, 5000, 5099);
    expect(read(200), count(100).at(0).noChanges());
    write(in2, 5000, 5099);
    expect(read(0), count(0).resync());
    expect(read(100), count(100).at(5000).noChanges());
}

// Finding 3
TEST_F(MultiReader2RegressionDm, LateInputBackBeforeAnyReadLeavesNoDataBehindADrain)
{
    setupBase();
    write(in1, 100, 2199);
    write(in2, 100, 2199);
    expect(drain(), {count(2200).at(0).noChanges(), count(0).noChanges()});
}

// Comparative review, spec finding 1: in2 found late while streaming still delivers its aligned backlog before the failure
TEST_F(MultiReader2RegressionDm, LateInputWhileStreamingDeliversItsBacklogBeforeFailing)
{
    setupBase();
    write(in1, 100, 2199);
    expect(
        read(200),
        count(100).at(0).level(Error).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, more than 2 s from the main input"));
    expect(avail(), count(0));
}

// Finding 6
TEST_F(MultiReader2RegressionDm, ConfigureWithEqualParamsReportsWhatItsZeroCountReadApplies)
{
    given().minReadCount(10);
    setupBase();
    expect(read(95), count(95).at(0).noChanges());
    gap(in2);
    expect(configure(same()), ok().resync());
}

// Finding 9
TEST_F(MultiReader2RegressionDm, RejoiningInputIsNotLateForTheMainBacklogBeforeTheRejoin)
{
    given().policy(Exclude).accepts().verdict(in2, Verdict::Reject);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    write(in1, 100, 2499);
    verdict(in2, Verdict::Accept);
    expect(configure(same()), ok().level(Ok).descChanged(in2).healthy(in2));
    write(in2, 2500, 2599);
    write(in1, 2500, 2599);
    expect(read(100), count(100).at(2500).noChanges());
}

// Finding 10
TEST_F(MultiReader2RegressionDm, MainJumpingBackWhileStreamingKeepsItsQueuedSamples)
{
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    write(in1, 0, 99);
    expect(read(0), count(0).noChanges());
    write(in2, 0, 99);
    expect(read(0), count(0).resync());
    expect(read(100), count(100).at(0).noChanges());
}

// Finding 11
TEST_F(MultiReader2RegressionDm, ReadDeliveringNothingOutsideStreamingReportsPacketOffsetZero)
{
    given().ports();
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    disconnect(in2);
    const Result result = read(0);
    expect(result, count(0).level(Error).inError(in2, InputError::Disconnected).msg("in2: not connected"));
    EXPECT_EQ(result.packetOffset, 0u);
}

// Finding 4: an input queues at most 32 MiB. in2's samples take 800 kB, so of its 10-sample packets of 8 MB it keeps four
// while in1 is silent; the rest is dropped, and in2's next kept packet is a gap. Skips, since the harness's read buffers
// hold small samples only
TEST_F(MultiReader2RegressionDm, AnInputQueuesAtMostItsMemoryLimit)
{
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    announce(in2, Value().dims({100000}));
    expect(read(0), count(0).descChanged(in2).size(in2, 800000));
    write(in2, 100, 199);
    write(in1, 100, 199);
    expect(skip(100), count(40).at(100).noChanges());
    write(in1, 200, 209);
    write(in2, 200, 209);
    expect(skip(10), count(0).resync());
    expect(skip(10), count(10).at(200).noChanges());
}

// A packet that jumps as it ends the cap queues one gap; the continuous packet after it queues none
TEST_F(MultiReader2RegressionDm, AJumpEndingTheMemoryLimitQueuesOneGap)
{
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    announce(in2, Value().dims({100000}));
    expect(read(0), count(0).descChanged(in2).size(in2, 800000));
    write(in2, 100, 199);
    write(in1, 100, 199);
    expect(skip(100), count(40).at(100).noChanges());
    write(in1, 200, 229);
    write(in2, 210, 219);
    write(in2, 220, 229);
    expect(skip(10), count(0).resync());
    expect(skip(10), count(10).at(210).noChanges());
    expect(skip(10), count(10).at(220).noChanges());
}

// Comparative review, debuggability finding 1: the status tells waiting for a common start from having no data
TEST_F(MultiReader2RegressionDm, StatusShowsSynchronizingUntilTheCommonStart)
{
    write(in1, 0, 99);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().synchronizing());
    expect(read(0), count(0).noChanges().synchronizing());
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges().synchronizing(false));
}

}
