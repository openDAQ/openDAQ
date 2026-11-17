#include "multi_reader2_harness.h"

namespace daq::mr2test
{

// Regression tests for the MultiReader2 data manager
class MultiReader2RegressionDm : public MultiReader2Test
{
};

TEST_F(MultiReader2RegressionDm, EventBehindDataDroppedBySyncIsReportedWithTheFirstData)
{
    given().main(in1);
    setup([&] { read(0, nullData); });
    handler([&] { drain(); });
    write(in2, 0, 99);
    announce(in2, Value().unit("A"));
    expect(idle(), calls(1, {count(0).noChanges()}));
    write(in1, 500, 599);
    expect(idle(), calls(0));
    write(in2, 500, 599);
    expect(idle(), calls(1, {count(0).descChanged(in2), count(100).at(500).noChanges(), count(0).noChanges()}));
}

TEST_F(MultiReader2RegressionDm, MainJumpingAheadWhileStreamingFailsNoOtherInput)
{
    setupBase();
    write(in1, 5000, 5099);
    expect(read(200), count(100).at(0).noChanges());
    write(in2, 5000, 5099);
    expect(read(0), count(0).resync());
    expect(read(100), count(100).at(5000).noChanges());
}

TEST_F(MultiReader2RegressionDm, DrainReadsTheWholeBacklogInOneCall)
{
    setupBase();
    write(in1, 100, 2199);
    write(in2, 100, 2199);
    expect(drain(), {count(2200).at(0).noChanges(), count(0).noChanges()});
}

// in2 delivers its aligned backlog before it fails; only an input with nothing queued stalls
TEST_F(MultiReader2RegressionDm, InputStallsOnlyOnceItsBacklogIsRead)
{
    setupBase();
    expect(read(50), count(50).at(0).noChanges());
    write(in1, 100, 3049);
    expect(read(200), count(50).at(50).noChanges());
    write(in1, 3050, 3109);
    expect(read(0), count(0).level(Error).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, no data for 3 s"));
    expect(avail(), count(0));
}

TEST_F(MultiReader2RegressionDm, ReevaluateReportsWhatItsZeroCountReadApplies)
{
    given().minReadCount(10);
    setupBase();
    expect(read(95), count(95).at(0).noChanges());
    write(in2, 200, 209);
    expect(reevaluate(), ok().resync());
}

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
    expect(reevaluate(), ok().level(Ok).descChanged(in2).healthy(in2));
    write(in2, 2500, 2599);
    write(in1, 2500, 2599);
    expect(read(100), count(100).at(2500).noChanges());
}

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

}
