#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Configure : public MultiReader2Test
{
};

// S1.CFG.01
TEST_F(MultiReader2Configure, ReaderWithoutInputsIsOkWithoutMainAndReadsZero)
{
    given().inputs(0);
    expect(read(0), count(0).level(Ok).mainNull());
    expect(read(10), count(0).noChanges());
}

// S1.CFG.02
TEST_F(MultiReader2Configure, ConfigureAddingInputToEmptyReaderReportsItAsMain)
{
    given().inputs(0);
    setup([&] { read(0, nullData); });
    expect(configure(same().add(in1)), ok().main(in1).domainChanged().resync().allDescChanged().inputs({in1}));
    write(in1, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.03
TEST_F(MultiReader2Configure, ConstructWithOneInputReportsItAsMainOnFirstRead)
{
    given().inputs(1);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged());
    write(in1, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.04
TEST_F(MultiReader2Configure, ConstructWithPinnedMainReportsItAsMainOnFirstRead)
{
    given().main(in2);
    expect(read(0), count(0).level(Ok).main(in2).domainChanged().resync().allDescChanged());
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.05
TEST_F(MultiReader2Configure, AutomaticMainSkipsDisconnectedFirstPortUnderExclude)
{
    given().ports().policy(Exclude).unconnected(in1);
    expect(read(0),
           count(0)
               .level(Warning)
               .main(in2)
               .domainChanged()
               .resync()
               .descChanged(in2)
               .inError(in1, InputError::Disconnected)
               .msg("in1: not connected"));
}

// S1.CFG.06
TEST_F(MultiReader2Configure, ConstructWithUnconnectedPortSucceedsWithDisconnectedError)
{
    given().ports().unconnected(in2);
    expectPort(in2, Listener::Reader, PacketReadyNotification::Scheduler);
    expect(read(0), count(0).level(Error).inError(in2, InputError::Disconnected).msg("in2: not connected"));
}

// S1.CFG.07
TEST_F(MultiReader2Configure, ReleasedReaderFreesItsPortsForANewReader)
{
    given().ports();
    setupBase();
    expectPort(in1, Listener::Reader, PacketReadyNotification::Scheduler);
    idle();
    release();
    expectDestroyed();
    expectPort(in1, Listener::None, PacketReadyNotification::None);
    expect(construct(same()), ok());
}

// S1.CFG.08
TEST_F(MultiReader2Configure, ConfigureAddingUnconnectedPortSucceedsWithDisconnectedError)
{
    given().ports().unconnected(in3);
    setupBase();
    expect(configure(same().add(in3)),
           ok().level(Error).inError(in3, InputError::Disconnected).msg("in3: not connected").inputs({in1, in2, in3}));
    expectPort(in3, Listener::Reader, PacketReadyNotification::Scheduler);
}

// S1.CFG.09
TEST_F(MultiReader2Configure, ConfigureWithEqualParamsKeepsQueuedDataWhileSynchronizing)
{
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same()), ok().noChanges());
    write(in2, 0, 99);
    expect(avail(), count(100));
}

// S1.CFG.10
TEST_F(MultiReader2Configure, ConfigureWithEqualParamsKeepsQueuedDataWhenSynced)
{
    setupBase();
    expect(configure(same()), ok().noChanges());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.11
TEST_F(MultiReader2Configure, ConfigureWithEqualParamsKeepsQueuedDataUnderWarning)
{
    given().policy(Exclude).value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same()), ok().noChanges());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.12
TEST_F(MultiReader2Configure, ConfigureWithEqualParamsUnderErrorRecoversOnAcceptedVerdict)
{
    given().accepts().verdict(in2, Verdict::Reject);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    verdict(in2, Verdict::Accept);
    expect(configure(same()), ok().level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2).stubAsked({in1, in2}));
}

// S1.CFG.13
TEST_F(MultiReader2Configure, ConfigureAddingInputWhileSynchronizingDropsQueuedData)
{
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().add(in3)), ok().domainChanged().resync().allDescChanged().inputs({in1, in2, in3}));
    write(in2, 0, 99);
    write(in3, 0, 99);
    expect(avail(), count(0));
}

// S1.CFG.14
TEST_F(MultiReader2Configure, ConfigureAddingInputWhenSyncedDropsQueuedData)
{
    setupBase();
    expect(configure(same().add(in3)), ok().domainChanged().resync().allDescChanged().inputs({in1, in2, in3}));
    write(in1, 100, 199);
    write(in2, 100, 199);
    write(in3, 100, 199);
    expect(read(100), count(100).at(100).noChanges());
}

// S1.CFG.15
TEST_F(MultiReader2Configure, ConfigureAddingInputUnderWarningKeepsFailingInputExcluded)
{
    given().policy(Exclude).value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().add(in3)), ok().domainChanged().resync().descChanged(in1).descChanged(in3).inputs({in1, in2, in3}));
    write(in3, 0, 99);
    expect(avail(), count(0));
}

// S1.CFG.16
TEST_F(MultiReader2Configure, ConfigureAddingInputUnderErrorStaysInError)
{
    given().value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().add(in3)), ok().inputs({in1, in2, in3}));
}

// S1.CFG.18
TEST_F(MultiReader2Configure, ConfigureRemovingPortReleasesItForANewReader)
{
    given().ports();
    setupBase();
    expect(configure(same().remove(in2)), ok().domainChanged().resync().allDescChanged().inputs({in1}));
    expect(avail(), count(0));
    expectPort(in2, Listener::None, PacketReadyNotification::None);
    expect(construct(same().remove(in1)), ok());
}

// S1.CFG.19
TEST_F(MultiReader2Configure, ConfigureRemovingExcludedInputUnderWarningReturnsOk)
{
    given().policy(Exclude).value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().remove(in2)), ok().level(Ok).domainChanged().resync().allDescChanged().inputs({in1}));
    expect(avail(), count(0));
}

// S1.CFG.20
TEST_F(MultiReader2Configure, ConfigureRemovingFailingInputUnderErrorRecoversToOk)
{
    given().value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().remove(in2)), ok().level(Ok).main(in1).domainChanged().resync().allDescChanged().inputs({in1}));
}

// S1.CFG.22
TEST_F(MultiReader2Configure, ConfigureChangingReadTypeReportsNewSampleSizes)
{
    setupBase();
    expect(configure(same().readType(SampleType::Int32)), ok().domainChanged().resync().allDescChanged().size(in1, 4).size(in2, 4));
    expect(avail(), count(0));
}

// S1.CFG.23
TEST_F(MultiReader2Configure, ConfigureChangingReadTypeUnderWarningUpdatesErrorMessage)
{
    given().policy(Exclude).value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().readType(SampleType::Int32)),
           ok().domainChanged().resync().descChanged(in1).msg("in2: value type not convertible to Int32").size(in1, 4));
    expect(avail(), count(0));
}

// S1.CFG.26
TEST_F(MultiReader2Configure, ConfigureAddingPortTakesOverBlockListener)
{
    given().ports().listenerBlock(in3, PacketReadyNotification::SameThread);
    setupBase();
    expect(configure(same().add(in3)), ok().domainChanged().resync().allDescChanged().inputs({in1, in2, in3}));
    expectPort(in3, Listener::Reader, PacketReadyNotification::Scheduler);
    expect(avail(), count(0));
}

// S1.CFG.30
TEST_F(MultiReader2Configure, ConfigureChangingPinnedMainReportsNewMain)
{
    given().main(in1);
    setupBase();
    expect(configure(same().main(in2)), ok().main(in2).domainChanged().resync().allDescChanged());
    expect(avail(), count(0));
}

// S1.CFG.31
TEST_F(MultiReader2Configure, ConfigurePinningExcludedInputAsMainReportsError)
{
    given().main(in1).policy(Exclude).value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().main(in2)), ok().level(Error).main(in2));
}

// S1.CFG.34
TEST_F(MultiReader2Configure, MutatingParamsAfterConfigureDoesNotAffectReader)
{
    setupBase();
    expect(configure(same()), ok().noChanges());
    mutate(same().minReadCount(200));
    expect(avail(), count(100));
}

// S1.CFG.38
TEST_F(MultiReader2Configure, ConfigureChangingMinReadCountAppliesNewThreshold)
{
    setupBase();
    expect(configure(same().minReadCount(150)), ok().domainChanged().resync().allDescChanged());
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(avail(), count(0));
    write(in1, 200, 249);
    write(in2, 200, 249);
    expect(avail(), count(150));
}

// S1.CFG.40
TEST_F(MultiReader2Configure, ConfigureChangingMinReadCountUnderErrorAppliesAfterRecovery)
{
    given().value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().minReadCount(150)), ok().hasChanges());
    announce(in2, Value());
    expect(read(0), count(0).level(Ok).domainChanged().resync().allDescChanged().healthy(in2));
    write(in1, 100, 199);
    write(in2, 100, 199);
    expect(avail(), count(0));
    write(in1, 200, 249);
    write(in2, 200, 249);
    expect(avail(), count(150));
}

// S1.CFG.42
TEST_F(MultiReader2Configure, ConfigureChangingPolicyWhenSyncedDropsQueuedData)
{
    setupBase();
    expect(configure(same().policy(Exclude)), ok().domainChanged().resync().allDescChanged());
    expect(avail(), count(0));
}

// S1.CFG.43
TEST_F(MultiReader2Configure, ConfigureSwitchingToInvalidateUnderWarningReportsError)
{
    given().policy(Exclude).value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().policy(Invalidate)), ok().level(Error).mainNull());
}

// S1.CFG.44
TEST_F(MultiReader2Configure, ConfigureSwitchingToExcludeUnderErrorReportsWarning)
{
    given().value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().policy(Exclude)), ok().level(Warning).main(in1).domainChanged().resync().descChanged(in1));
}

// S1.CFG.46
TEST_F(MultiReader2Configure, ConfigureReplacingAcceptsDescriptorAsksEveryInput)
{
    given().accepts().verdict(in2, Verdict::Accept);
    setupBase();
    expect(configure(same().newAccepts()), ok().domainChanged().resync().allDescChanged().stubAsked({in1, in2}));
    expect(avail(), count(0));
}

// S1.CFG.47
TEST_F(MultiReader2Configure, ConfigureReplacingAcceptsDescriptorUnderWarningRejoinsInput)
{
    given().accepts().policy(Exclude).verdict(in2, Verdict::Reject);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    verdict(in2, Verdict::Accept);
    expect(configure(same().newAccepts()), ok().level(Ok).domainChanged().resync().allDescChanged().healthy(in2).stubAsked({in1, in2}));
    write(in2, 0, 99);
    expect(avail(), count(0));
}

// S1.CFG.48
TEST_F(MultiReader2Configure, ConfigureReplacingAcceptsDescriptorUnderErrorRecoversReader)
{
    given().accepts().verdict(in2, Verdict::Reject);
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    verdict(in2, Verdict::Accept);
    expect(configure(same().newAccepts()),
           ok().level(Ok).main(in1).domainChanged().resync().allDescChanged().healthy(in2).stubAsked({in1, in2}));
}

// S1.CFG.50
TEST_F(MultiReader2Configure, ConfigureReversingTenInputsMakesNewFirstInputMain)
{
    given().inputs(10);
    setupBase();
    expect(configure(same().order({in10, in9, in8, in7, in6, in5, in4, in3, in2, in1})),
           ok().main(in10).domainChanged().resync().allDescChanged().inputs({in10, in9, in8, in7, in6, in5, in4, in3, in2, in1}));
    expect(avail(), count(0));
}

// S1.CFG.51
TEST_F(MultiReader2Configure, ConfigureReorderingUnderWarningKeepsFirstHealthyInputAsMain)
{
    given().policy(Exclude).value(in2, Value::structure());
    setup(
        [&]
        {
            write(in1, 0, 99);
            read(0, nullData);
        });
    expect(configure(same().order({in2, in1})), ok().domainChanged().resync().descChanged(in1).inputs({in2, in1}));
    expect(avail(), count(0));
}

// S1.CFG.53
TEST_F(MultiReader2Configure, ConfigureWithMainNotInListFailsAndLeavesReaderUnchanged)
{
    setupBase();
    expect([&] { return configure(same().main(in9)); }, failsAny().unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.54
TEST_F(MultiReader2Configure, ConstructWithMainNotInListFails)
{
    setupBase();
    expect([&] { return construct(same().main(in9)); }, failsAny().unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.55
TEST_F(MultiReader2Configure, ConfigureWithDuplicateInputFailsAndLeavesReaderUnchanged)
{
    setupBase();
    expect([&] { return configure(same().add(in1)); }, failsAny().unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.56
TEST_F(MultiReader2Configure, ConstructWithDuplicateInputFails)
{
    setupBase();
    expect([&] { return construct(same().add(in1)); }, failsAny().unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.57
TEST_F(MultiReader2Configure, ConfigureMixingSignalsAndPortsFailsAndLeavesReaderUnchanged)
{
    given().ports();
    setupBase();
    expect([&] { return configure(same().addSignal(s3)); }, failsAny().unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.58
TEST_F(MultiReader2Configure, ConstructMixingSignalsAndPortsFailsWithoutTakingOverPorts)
{
    given().ports();
    setupBase();
    idle();
    release();
    expectDestroyed();
    expect([&] { return construct(same().addSignal(s3)); }, failsAny().unchanged());
    expectPort(in1, Listener::None, PacketReadyNotification::None);
}

// S1.CFG.59
TEST_F(MultiReader2Configure, ConfigureWithPortHeldByOtherReaderFailsWithAlreadyExists)
{
    given().ports().heldByStreamReader(in3);
    setupBase();
    expect([&] { return configure(same().add(in3).add(in4)); }, fails(OPENDAQ_ERR_ALREADYEXISTS).unchanged());
    expectPort(in3, Listener::Other);
    expectPort(in4, Listener::None, PacketReadyNotification::None);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.60
TEST_F(MultiReader2Configure, ConstructWithPortHeldByOtherReaderFailsWithAlreadyExists)
{
    given().ports();
    setupBase();
    expect([&] { return construct(same().add(in3)); }, fails(OPENDAQ_ERR_ALREADYEXISTS).unchanged());
    expectPort(in1, Listener::Reader, PacketReadyNotification::Scheduler);
    expectPort(in3, Listener::None);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.61
TEST_F(MultiReader2Configure, ConfigureWithoutSchedulerFailsWithInvalidState)
{
    given().noScheduler(in3);
    setupBase();
    expect([&] { return configure(same().remove(in1).remove(in2).add(in3)); }, fails(OPENDAQ_ERR_INVALIDSTATE).unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.62
TEST_F(MultiReader2Configure, ConstructWithoutSchedulerFailsWithInvalidState)
{
    given().noScheduler(in3);
    setupBase();
    expect([&] { return construct(same().remove(in1).remove(in2).add(in3)); }, fails(OPENDAQ_ERR_INVALIDSTATE).unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.63
TEST_F(MultiReader2Configure, ConfigureWithZeroMinReadCountFailsAndLeavesReaderUnchanged)
{
    setupBase();
    expect([&] { return configure(same().minReadCount(0)); }, failsAny().unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.64
TEST_F(MultiReader2Configure, ConstructWithZeroMinReadCountFails)
{
    setupBase();
    expect([&] { return construct(same().minReadCount(0)); }, failsAny().unchanged());
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.65
TEST_F(MultiReader2Configure, ConstructTakesOverBlockListenerOfPort)
{
    given().ports().listenerBlock(in2, PacketReadyNotification::SameThread);
    expectPort(in2, Listener::Reader, PacketReadyNotification::Scheduler);
    expect(read(0), count(0).level(Ok).main(in1).domainChanged().resync().allDescChanged());
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.66
TEST_F(MultiReader2Configure, ConfigureWithEqualParamsBeforeFirstReadReturnsFirstStatus)
{
    expect(configure(same()), ok().level(Ok).domainChanged().resync().allDescChanged());
    expect(read(0), count(0).noChanges());
    write(in1, 0, 99);
    write(in2, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

// S1.CFG.67
TEST_F(MultiReader2Configure, ConfigureAddingInputBeforeFirstReadReturnsFirstStatus)
{
    expect(configure(same().add(in3)), ok().level(Ok).domainChanged().resync().allDescChanged().inputs({in1, in2, in3}));
    expect(read(0), count(0).noChanges());
    write(in1, 0, 99);
    write(in2, 0, 99);
    write(in3, 0, 99);
    expect(read(100), count(100).at(0).noChanges());
}

}
