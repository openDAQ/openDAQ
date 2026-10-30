#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Limits : public MultiReader2Test
{
};

// S1.LIM.01
TEST_F(MultiReader2Limits, ByteLimitDropsTheInputsQueuedDataAndReadsGoOnFromTheNewest)
{
    setupBase([&] { read(100); });
    announce(in2, Value().dims({4000000}));
    expect(read(0), count(0).descChanged(in2).size(in2, 32000000));
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
    announce(in2, Value().dims({4000000}));
    expect(read(0), count(0).descChanged(in2).size(in2, 32000000));
    write(in2, 100, 109);
    announce(in2, Value().dims({4000000}).unit("mV"));
    write(in2, 110, 199);
    write(in1, 100, 199);
    expect(skip(100), count(0).resync().descChanged(in2));
    expect(skip(100), count(10).at(190).noChanges());
}

// S1.LIM.09
TEST_F(MultiReader2Limits, ByteLimitDropsTheLargestQueueWhenAnotherInputPassesIt)
{
    // in2 holds 960 MB; in1's 80 MB packet passes the 1 GB the inputs share, and in2, holding the most, loses its data
    setupBase([&] { read(100); });
    announce(in1, Value().dims({1000000}));
    announce(in2, Value().dims({4000000}));
    expect(read(0), count(0).descChanged(in1).descChanged(in2).size(in1, 8000000).size(in2, 32000000));
    write(in2, 100, 129);
    write(in1, 100, 109);
    write(in2, 130, 139);
    write(in1, 110, 139);
    expect(skip(100), count(0).resync());
    expect(skip(100), count(10).at(130).noChanges());
}

// S1.LIM.03
TEST_F(MultiReader2Limits, PinnedMainInputStalledFourSecondsBehindInvalidatesReaderUntilItSendsAgain)
{
    given().policy(Exclude).main(in1);
    setupBase([&] { read(100); });
    write(in2, 100, 4099);
    expect(read(0), count(0).noChanges());
    write(in2, 4100, 4109);
    expect(read(0),
           count(0)
               .level(Error)
               .main(in1)
               .inError(in1, InputError::SyncFailed)
               .msg("in1: not synchronized, more than 4 s behind the other inputs"));
    write(in1, 100, 199);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in1).msg(""));
    write(in1, 4110, 4209);
    write(in2, 4110, 4209);
    expect(read(100), count(100).at(4110).noChanges());
}

// S1.LIM.04
TEST_F(MultiReader2Limits, AutomaticMainInputStalledMovesToTheNextInput)
{
    given().policy(Exclude);
    setupBase([&] { read(100); });
    write(in2, 100, 4109);
    expect(read(0), count(0).level(Ok).main(in2).domainChanged().resync().allDescChanged());
    write(in2, 4110, 6120);
    expect(read(0),
           count(0).level(Warning).inError(in1, InputError::SyncFailed).msg("in1: not synchronized, more than 2 s from the main input"));
    expect(read(100), count(100).at(4110).noChanges());
}

// S1.LIM.05
TEST_F(MultiReader2Limits, StallIsWithdrawnWhenTheMainInputSendsBeforeARead)
{
    given().policy(Exclude).main(in1);
    setupBase([&] { read(100); });
    write(in2, 100, 4109);
    write(in1, 100, 4109);
    expect(read(100), count(0).resync());
    expect(read(100), count(10).at(4100).noChanges());
}

// S1.LIM.06
TEST_F(MultiReader2Limits, SilentPinnedMainInputStallsWhileSynchronizing)
{
    given().policy(Exclude).main(in1);
    write(in2, 0, 4000);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged());
    write(in2, 4001, 4010);
    expect(read(0),
           count(0)
               .level(Error)
               .main(in1)
               .inError(in1, InputError::SyncFailed)
               .msg("in1: not synchronized, more than 4 s behind the other inputs"));
    write(in1, 0, 99);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in1).msg(""));
    write(in1, 4011, 4110);
    write(in2, 4011, 4110);
    expect(read(100), count(100).at(4011).noChanges());
}

// S1.LIM.07
TEST_F(MultiReader2Limits, TimeLimitDropsTheQueuedDataPastFourSeconds)
{
    setupBase([&] { read(100); });
    write(in1, 100, 4099);
    write(in2, 100, 4099);
    expect(avail(), count(4000));
    write(in1, 4100, 4109);
    write(in2, 4100, 4109);
    expect(read(100), count(0).resync());
    expect(read(100), count(10).at(4100).noChanges());
}

// S1.LIM.08
TEST_F(MultiReader2Limits, InputWithoutDataFailsWhileSynchronizingPastTheQueueLimit)
{
    given().policy(Exclude).value(in1, Value().dims({4000000})).domain(in1, Domain().delta(50)).domain(in2, Domain().delta(50));
    write(in1, 0, 2050);
    expect(read(0),
           count(0)
               .level(Warning)
               .main(in1)
               .domainChanged()
               .resync()
               .allDescChanged()
               .inError(in2, InputError::SyncFailed)
               .msg("in2: not synchronized, more than 2 s from the main input"));
}

}
