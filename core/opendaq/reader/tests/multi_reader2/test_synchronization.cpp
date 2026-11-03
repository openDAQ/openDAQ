#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Sync : public MultiReader2Test
{
};

// S1.SYNC.01
TEST_F(MultiReader2Sync, InputsStartingTogetherReadFromFirstTickAfterStatus)
{
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(avail(), count(0));
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.SYNC.02
TEST_F(MultiReader2Sync, PinnedMainInputsStartingTogetherReadFromFirstTick)
{
    given().main(in2);
    write(in2, 0, 99);
    write(in1, 0, 99);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.SYNC.03
TEST_F(MultiReader2Sync, NonMainStartingLaterSetsCommonStartAndMainDropsEarlierData)
{
    write(in1, 0, 99);
    write(in2, 50, 149);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(avail(), count(50));
    expect(read(50), count(50).at(50).noChanges());
}

// S1.SYNC.04
TEST_F(MultiReader2Sync, PinnedMainStartingLaterDropsNonMainDataBeforeCommonStart)
{
    given().main(in2);
    write(in2, 50, 149);
    write(in1, 0, 99);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(avail(), count(50));
    expect(read(50), count(50).at(50).noChanges());
}

// S1.SYNC.05
TEST_F(MultiReader2Sync, NonMainExactlyQueueLimitAheadSynchronizes)
{
    MR2_REVIEW_ONLY(review_only::syncLimit);
    write(in1, 0, 99);
    write(in2, 3099, 3198);
    write(in1, 100, 3199);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(read(100), count(100).at(3099).noChanges());
}

// S1.SYNC.06
TEST_F(MultiReader2Sync, NonMainBehindPinnedMainSynchronizesOnceItCatchesUp)
{
    given().main(in2);
    write(in1, 0, 99);
    write(in2, 3099, 3198);
    write(in1, 100, 3198);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(read(100), count(100).at(3099).noChanges());
}

// S1.SYNC.07
TEST_F(MultiReader2Sync, NonMainMoreThanQueueLimitAheadIsExcludedWithWarning)
{
    given().policy(Exclude);
    write(in1, 0, 99);
    write(in2, 3100, 3199);
    expect(read(0),
           count(0)
               .level(Warning)
               .main(in1)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::SyncFailed)
               .msg("in2: not synchronized, more than 3 s ahead of the main input"));
    expect(read(100), count(100).at(0).noChanges());
}

// S1.SYNC.08
TEST_F(MultiReader2Sync, NonMainMoreThanQueueLimitAheadOfPinnedMainGivesError)
{
    given().main(in2);
    write(in2, 0, 99);
    write(in1, 3100, 3199);
    expect(read(0),
           count(0).level(Error).inError(in1, InputError::SyncFailed).msg("in1: not synchronized, more than 3 s ahead of the main input"));
    expect(avail(), count(0));
}

// S1.SYNC.09
TEST_F(MultiReader2Sync, NonMainBehindMainStallsOnceTheMainSentThreeSecondsPastIt)
{
    MR2_REVIEW_ONLY(review_only::stall);
    write(in2, 0, 99);
    write(in1, 2100, 3099);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    write(in1, 3100, 3109);
    expect(read(0), count(0).level(Error).mainNull().inError(in2, InputError::SyncFailed).msg("in2: not synchronized, no data for 3 s"));
    expect(avail(), count(0));
}

// S1.SYNC.10
TEST_F(MultiReader2Sync, NonMainBehindPinnedMainIsStalledAndExcludedWithWarning)
{
    MR2_REVIEW_ONLY(review_only::stall);
    given().main(in2).policy(Exclude);
    write(in1, 0, 99);
    write(in2, 2100, 3109);
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in2)
               .inError(in1, InputError::SyncFailed)
               .msg("in1: not synchronized, no data for 3 s"));
    expect(read(100), count(100).at(2100).noChanges());
}

// S1.SYNC.11
TEST_F(MultiReader2Sync, NonMainJumpAheadMidStreamResynchronizesAtNewCommonStart)
{
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    write(in2, 500, 599);
    write(in1, 100, 599);
    expect(avail(), count(0));
    expect(read(0), count(0).resync());
    expect(read(100), count(100).at(500).noChanges());
}

// S1.SYNC.12
TEST_F(MultiReader2Sync, PinnedMainJumpAheadMidStreamResynchronizesAtNewCommonStart)
{
    given().main(in2);
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    write(in2, 500, 599);
    write(in1, 100, 599);
    expect(avail(), count(0));
    expect(read(0), count(0).resync());
    expect(read(100), count(100).at(500).noChanges());
}

// S1.SYNC.13
TEST_F(MultiReader2Sync, NonMainJumpBackMidStreamContinuesWithoutResynchronized)
{
    MR2_REVIEW_ONLY(review_only::jumpWhileStreaming);
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    write(in2, 50, 199);
    write(in1, 100, 199);
    expect(avail(), count(0));
    expect(read(200), count(100).at(100).noChanges());
}

// S1.SYNC.14
TEST_F(MultiReader2Sync, PinnedMainJumpBackMidStreamContinuesWithoutResynchronized)
{
    MR2_REVIEW_ONLY(review_only::jumpWhileStreaming);
    given().main(in2);
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    write(in2, 50, 199);
    write(in1, 100, 199);
    expect(avail(), count(0));
    expect(read(200), count(100).at(100).noChanges());
}

// S1.SYNC.15
TEST_F(MultiReader2Sync, NonMainJumpMoreThanQueueLimitAheadMidStreamFailsSync)
{
    MR2_REVIEW_ONLY(review_only::syncLimit);
    given().policy(Exclude);
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    write(in1, 100, 199);
    write(in2, 3200, 3299);
    expect(avail(), count(0));
    expect(
        read(0),
        count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, more than 3 s ahead of the main input"));
    expect(read(100), count(100).at(100).noChanges());
}

// S1.SYNC.16
TEST_F(MultiReader2Sync, NonMainWithoutDataHoldsReadsWhileMainWithinQueueLimit)
{
    setup(
        [&]
        {
            write(in1, 0, 1999);
            read(0, nullData);
        });
    expect(avail(), count(0));
    expect(read(0), count(0).noChanges());
    write(in2, 0, 1999);
    expect(avail(), count(2000));
    expect(read(2000), count(2000).at(0).noChanges());
}

// S1.SYNC.17
TEST_F(MultiReader2Sync, NonMainWithoutDataIsExcludedOnceMainHoldsMoreThanQueueLimit)
{
    given().policy(Exclude);
    write(in1, 0, 3000);
    expect(read(0),
           count(0)
               .level(Warning)
               .domainChanged()
               .resync()
               .descChanged(in1)
               .inError(in2, InputError::SyncFailed)
               .msg("in2: not synchronized, no data for 3 s"));
    expect(read(3001), count(3001).at(0).noChanges());
}

// S1.SYNC.18
TEST_F(MultiReader2Sync, NonMainWithoutDataGivesErrorOnceMainHoldsMoreThanQueueLimit)
{
    write(in1, 0, 3000);
    expect(read(0), count(0).level(Error).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, no data for 3 s"));
    expect(avail(), count(0));
}

// S1.SYNC.19
TEST_F(MultiReader2Sync, NonMainWithoutDataSurvivesMainAtExactlyQueueLimitThenFails)
{
    MR2_REVIEW_ONLY(review_only::syncLimit);
    given().policy(Exclude);
    write(in1, 0, 2999);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(avail(), count(0));
    write(in1, 3000, 3000);
    expect(read(0), count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, no data for 3 s"));
    expect(read(3001), count(3001).at(0).noChanges());
}

// S1.SYNC.20
TEST_F(MultiReader2Sync, AvailableCountStaysZeroUntilLastOfTenInputsDelivers)
{
    given().inputs(10);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(read(0), count(0).noChanges());
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
    expect(read(100), count(100).at(0).noChanges());
}

// S1.SYNC.21
TEST_F(MultiReader2Sync, NoDataOnMainHoldsReadsWithoutFailingNonMain)
{
    setup(
        [&]
        {
            write(in2, 0, 99);
            read(0, nullData);
        });
    write(in2, 100, 2999);
    expect(avail(), count(0));
    expect(read(0), count(0).noChanges());
    write(in1, 0, 99);
    expect(avail(), count(100));
    expect(read(100), count(100).at(0).noChanges());
}

// S1.SYNC.22
TEST_F(MultiReader2Sync, MainWaitsForDataUpToLaterNonMainCommonStart)
{
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    write(in2, 1000, 1099);
    expect(avail(), count(0));
    expect(read(0), count(0).noChanges());
    write(in1, 100, 1099);
    expect(avail(), count(100));
    expect(read(100), count(100).at(1000).noChanges());
}

// S1.SYNC.23
TEST_F(MultiReader2Sync, NonMainWaitingBehindEventIsNotStalledWhileMainExceedsQueueLimit)
{
    MR2_REVIEW_ONLY(review_only::queueLimit);
    // in2's queued event is not nothing: in1 drops its oldest packets to stay within 3 s instead
    announce(in2, Value(SampleType::Int32));
    write(in1, 0, 3049);
    write(in2, 0, 99);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(read(100), count(50).at(50).noChanges());
}

// S1.SYNC.24
TEST_F(MultiReader2Sync, NonMainAheadIsNotJudgedWhileMainWaitsBehindEvent)
{
    MR2_REVIEW_ONLY(review_only::syncLimit);
    announce(in1, Value(SampleType::Int32));
    write(in1, 0, 99);
    write(in2, 3100, 3199);
    write(in1, 100, 199);
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged());
    expect(avail(), count(0));
    write(in1, 200, 3199);
    expect(avail(), count(100));
    expect(read(100), count(100).at(3100).noChanges());
}

// S1.SYNC.25
TEST_F(MultiReader2Sync, NonMainOffDeltaTwoMainGridIsExcludedAsOffGrid)
{
    given().policy(Exclude).signal(in1, sig::delta2()).signal(in2, sig::delta2());
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
    expect(read(100), count(100).at(2).noChanges());
}

// S1.SYNC.26
TEST_F(MultiReader2Sync, NonMainStoppingWhileStreamingStallsAndRecoversWithinOneSecond)
{
    MR2_REVIEW_ONLY(review_only::stall);
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    write(in1, 100, 3099);
    expect(avail(), count(0));
    expect(read(0), count(0).noChanges());
    write(in1, 3100, 3199);
    expect(read(0), count(0).level(Error).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, no data for 3 s"));
    write(in2, 100, 199);
    expect(read(0), count(0).noChanges());
    write(in2, 2200, 2299);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged());
    write(in1, 3200, 3299);
    write(in2, 3200, 3299);
    expect(read(100), count(100).at(3200).noChanges());
}

// S1.SYNC.29
TEST_F(MultiReader2Sync, NonMainStoppingWhileStreamingIsSetAsideAndRejoinsUnderExclude)
{
    MR2_REVIEW_ONLY(review_only::stall);
    given().policy(Exclude);
    setupBase();
    expect(read(100), count(100).at(0).noChanges());
    write(in1, 100, 3199);
    expect(read(0), count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, no data for 3 s"));
    expect(read(3100), count(3100).at(100).noChanges());
    write(in2, 2200, 2299);
    write(in1, 3200, 3299);
    // The packet that brings in2 within 1 s rejoins it; its data before the read position is trimmed, so reading goes on
    // from there once it catches up
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    expect(read(100), count(0).noChanges());
    write(in2, 2300, 3299);
    expect(read(100), count(100).at(3200).noChanges());
}

// S1.SYNC.30
TEST_F(MultiReader2Sync, LosingTheAutomaticMainJudgesALateInputAfreshAgainstTheNewMain)
{
    given().ports().policy(Exclude);
    setup(
        [&]
        {
            write(in1, 0, 99);
            write(in2, 3100, 3199);
            read(0, nullData);
            read(100);
        });
    disconnect(in1);
    expect(read(0),
           count(0)
               .level(Warning)
               .main(in2)
               .domainChanged()
               .resync()
               .descChanged(in2)
               .inError(in1, InputError::Disconnected)
               .msg("in1: not connected"));
    write(in2, 3200, 3299);
    expect(read(100), count(100).at(3200).noChanges());  // the reconfigure dropped what was queued
}

// S1.SYNC.27
TEST_F(MultiReader2Sync, AheadSyncFailedInputRejoinsWithinOneSecondWithResynchronization)
{
    MR2_REVIEW_ONLY(review_only::stall);
    given().policy(Exclude);
    setup(
        [&]
        {
            write(in1, 0, 99);
            write(in2, 3100, 3199);
            read(0, nullData);
            read(100);
        });
    write(in1, 100, 2199);
    write(in2, 3200, 3209);  // 1.01 s ahead of in1: stays out
    expect(read(2100), count(2100).at(100).noChanges());
    write(in1, 2200, 2299);
    write(in2, 3210, 3299);  // 0.92 s ahead: rejoins, the packets after the one that rejoined it are kept
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2));
    write(in1, 2300, 3299);
    expect(read(100), count(0).resync());
    expect(read(100), count(80).at(3220).noChanges());
}

// S1.SYNC.28
TEST_F(MultiReader2Sync, NonMainWithNullValueDescriptorIsExcludedAndMainReads)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    announce(in2, Value::null());
    expect(avail(), count(0));
    expect(read(0), count(0).level(Warning).inError(in2, InputError::ValueDescriptorInvalid).msg("in2: no value descriptor"));
    expect(avail(), count(100));
    expect(read(100), count(100).at(0).noChanges());
}

}
