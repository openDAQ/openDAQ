#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Limits : public MultiReader2Test
{
};

// S1.LIM.01
TEST_F(MultiReader2Limits, ByteLimitDropsTheInputsQueuedDataAndReadsGoOnFromTheNewest)
{
    // in2's 160 MB packets each pass the 200 MB the two inputs share together with the one before
    setupBase([&] { read(100); });
    announce(in2, Value().dims({2000000}));
    expect(read(0), count(0).descChanged(in2).size(in2, 16000000));
    write(in2, 100, 199);
    write(in1, 100, 199);
    expect(skip(100), count(0).resync());
    expect(skip(100), count(10).at(190).noChanges());
    write(in1, 200, 209);
    write(in2, 200, 209);
    expect(skip(100), count(10).at(200).noChanges());
}

// S1.LIM.02
TEST_F(MultiReader2Limits, ByteLimitKeepsTheEventsQueuedOnTheClearedInput)
{
    setupBase([&] { read(100); });
    announce(in2, Value().dims({2000000}));
    expect(read(0), count(0).descChanged(in2).size(in2, 16000000));
    write(in2, 100, 109);
    announce(in2, Value().dims({2000000}).unit("mV"));
    write(in2, 110, 199);
    write(in1, 100, 199);
    expect(skip(100), count(0).resync().descChanged(in2));
    expect(skip(100), count(10).at(190).noChanges());
}

// S1.LIM.09
TEST_F(MultiReader2Limits, ByteLimitDropsTheLargestQueueWhenAnotherInputPassesIt)
{
    // in2 holds 192 MB; in1's 16 MB packet passes the 200 MB the inputs share, and in2, holding the most, loses its data
    setupBase([&] { read(100); });
    announce(in1, Value().dims({200000}));
    announce(in2, Value().dims({800000}));
    expect(read(0), count(0).descChanged(in1).descChanged(in2).size(in1, 1600000).size(in2, 6400000));
    write(in2, 100, 129);
    write(in1, 100, 109);
    write(in2, 130, 139);
    write(in1, 110, 139);
    expect(skip(100), count(0).resync());
    expect(skip(100), count(10).at(130).noChanges());
}

// S1.LIM.03
TEST_F(MultiReader2Limits, PinnedMainInputStalledInvalidatesReaderUntilItIsBackWithinOneSecond)
{
    given().policy(Exclude).main(in1);
    setupBase([&] { read(100); });
    write(in2, 100, 3099);
    expect(read(0), count(0).noChanges());
    write(in2, 3100, 3109);
    expect(read(0), count(0).level(Error).main(in1).inError(in1, InputError::SyncFailed).msg("in1: not synchronized, no data for 3 s"));
    write(in1, 100, 199);
    expect(read(0), count(0).noChanges());  // 2.9 s behind in2: not yet back
    write(in1, 2200, 2299);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in1).msg(""));
    write(in1, 3110, 3209);
    write(in2, 3110, 3209);
    expect(read(100), count(100).at(3110).noChanges());
}

// S1.LIM.04
TEST_F(MultiReader2Limits, AutomaticMainInputStalledMovesToTheNextInput)
{
    given().policy(Exclude);
    setupBase([&] { read(100); });
    write(in2, 100, 3109);
    expect(read(0),
           count(0)
               .level(Warning)
               .main(in2)
               .domainChanged()
               .resync()
               .descChanged(in2)
               .inError(in1, InputError::SyncFailed)
               .msg("in1: not synchronized, no data for 3 s"));
    write(in2, 3110, 3209);
    expect(read(100), count(100).at(3110).noChanges());
}

// S1.LIM.05
TEST_F(MultiReader2Limits, NoInputStallsWhenTheSilentInputSendsBeforeARead)
{
    // in2 keeps its data past the limit while in1 has nothing; in1's own data then passes the limit and is dropped
    given().policy(Exclude).main(in1);
    setupBase([&] { read(100); });
    write(in2, 100, 4109);
    write(in1, 100, 4109);
    expect(read(100), count(0).resync());
    expect(read(100), count(100).at(3100).noChanges());
}

// S1.LIM.06
TEST_F(MultiReader2Limits, SilentPinnedMainInputStallsWhileSynchronizing)
{
    given().policy(Exclude).main(in1);
    write(in2, 0, 2999);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged());
    write(in2, 3000, 3009);
    expect(read(0), count(0).level(Error).main(in1).inError(in1, InputError::SyncFailed).msg("in1: not synchronized, no data for 3 s"));
    write(in1, 2010, 2109);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in1).msg(""));
    write(in1, 3010, 3109);
    write(in2, 3010, 3109);
    expect(read(100), count(100).at(3010).noChanges());
}

// S1.LIM.07
TEST_F(MultiReader2Limits, TimeLimitDropsTheQueuedDataPastThreeSeconds)
{
    setupBase([&] { read(100); });
    write(in1, 100, 3099);
    write(in2, 100, 3099);
    expect(avail(), count(3000));
    write(in1, 3100, 3109);
    write(in2, 3100, 3109);
    expect(read(100), count(0).resync());
    expect(read(100), count(10).at(3100).noChanges());
}

// S1.LIM.08
TEST_F(MultiReader2Limits, InputWithoutDataStallsWhileSynchronizingAlthoughTheByteLimitTrimsTheOther)
{
    // in1's packets pass the byte limit on their own, so its queue never holds 3 s; the ticks it sent still count
    given().policy(Exclude).value(in1, Value().dims({4000000})).domain(in1, Domain().delta(50)).domain(in2, Domain().delta(50));
    write(in1, 0, 3050);
    expect(read(0),
           count(0)
               .level(Warning)
               .main(in1)
               .domainChanged()
               .resync()
               .allDescChanged()
               .inError(in2, InputError::SyncFailed)
               .msg("in2: not synchronized, no data for 3 s"));
}

// S1.LIM.10
TEST_F(MultiReader2Limits, StalledInputDelayedBetweenOneAndThreeSecondsStaysOut)
{
    given().policy(Exclude);
    setupBase([&] { read(100); });
    write(in1, 100, 3109);
    expect(read(0), count(0).level(Warning).inError(in2, InputError::SyncFailed).msg("in2: not synchronized, no data for 3 s"));
    expect(read(3010), count(3010).at(100).noChanges());
    write(in2, 1110, 1209);  // 1.9 s behind in1
    write(in1, 3110, 3209);
    expect(read(100), count(100).at(3110).noChanges());
    write(in2, 2210, 2309);  // 0.9 s behind in1
    // Its data before the read position is trimmed, so reading goes on from there once it catches up
    expect(read(0), count(0).level(Ok).descChanged(in2).healthy(in2).msg(""));
    write(in2, 2310, 3309);
    write(in1, 3210, 3309);
    expect(read(100), count(100).at(3210).noChanges());
}
}
