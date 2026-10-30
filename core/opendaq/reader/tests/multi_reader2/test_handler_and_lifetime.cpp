#include "multi_reader2_harness.h"

namespace daq::mr2test
{

class MultiReader2Context : public MultiReader2Test
{
};

// S1.CTX.01
TEST_F(MultiReader2Context, HandlerWaitsUntilMinReadCountSamplesAreOnAllInputs)
{
    given().minReadCount(10);
    setupHandler([&] { drain(); });
    write(in1, 100, 109);
    write(in2, 100, 104);
    expect(idle(), calls(0));
    write(in2, 105, 109);
    expect(idle(), calls(1, {count(10).at(100).noChanges(), count(0).noChanges()}));
}

// S1.CTX.02
TEST_F(MultiReader2Context, HandlerIsCalledWithNullSender)
{
    setupHandler(
        [&]
        {
            sender();
            drain();
        });
    blockWorker();
    write(in1, 100, 199);
    write(in2, 100, 199);
    unblockWorker();
    expect(idle(), calls(1, {ok().nullSender(), count(100).at(100).noChanges(), count(0).noChanges()}));
}

// S1.CTX.03
TEST_F(MultiReader2Context, DataQueuedWhileCallIsPendingIsHandledByOneCall)
{
    setupHandler([&] { drain(); });
    blockWorker();
    write(in1, 100, 199);
    write(in2, 100, 199);
    write(in1, 200, 299);
    write(in2, 200, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(200).at(100).noChanges(), count(0).noChanges()}));
}

// S1.CTX.04
TEST_F(MultiReader2Context, HandlerLeavingSamplesUnreadIsCalledAgain)
{
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).noChanges()}));
}

// S1.CTX.05
TEST_F(MultiReader2Context, ThrowingHandlerIsLoggedAndCalledAgainOnlyOnNewData)
{
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    expectLoggedError();
    write(in1, 300, 309);
    expect(idle(), calls(1, {count(50).at(250).noChanges()}));
}

// S1.CTX.06
TEST_F(MultiReader2Context, SynchronizationReachingCommonStartTriggersHandler)
{
    given().minReadCount(200);
    setup(
        [&]
        {
            handler([&] { drain(); });
            write(in1, 0, 99);
            read(0, nullData);
            idle();
        });
    write(in2, 0, 99);
    expect(idle(), calls(1, {count(0).noChanges()}));
}

// S1.CTX.07
TEST_F(MultiReader2Context, SyncFailureTriggersHandlerWithWarning)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            handler([&] { drain(); });
            write(in1, 0, 99);
            read(0, nullData);
            idle();
        });
    write(in2, 2200, 2299);
    expect(idle(), calls(1, {count(0).level(Warning), count(100).at(0).noChanges(), count(0).noChanges()}));
}

// S1.CTX.08
TEST_F(MultiReader2Context, SyncFailedInputComingWithinLimitTriggersHandlerOnce)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            handler([&] { drain(); });
            write(in1, 2500, 2599);
            write(in2, 0, 99);
            idle();
        });
    write(in2, 100, 699);
    expect(idle(), calls(1, {count(0).level(Ok).descChanged(in2).healthy(in2), count(0).noChanges()}));
}

// S1.CTX.09
TEST_F(MultiReader2Context, SyncFailedRejoinTriggerIsRearmedAfterHandlerThrows)
{
    given().policy(Exclude);
    setup(
        [&]
        {
            handler([&] { throw std::runtime_error("handler throws"); });
            write(in1, 2500, 2599);
            write(in2, 0, 99);
            idle();
            read(0, nullData);
            read(100);
            read(0, nullData);
        });
    blockWorker();
    write(in2, 100, 699);
    unblockWorker();
    expect(idle(), calls(1));
    write(in2, 700, 709);
    expect(idle(), calls(1));
}

// S1.CTX.10
TEST_F(MultiReader2Context, DescriptorChangeCallsHandlerWhileReaderIsInvalid)
{
    given().accepts().verdict(in2, Verdict::Reject);
    setup(
        [&]
        {
            handler([&] { drain(); });
            write(in1, 0, 99);
            write(in2, 0, 99);
            read(0, nullData);
            idle();
        });
    announce(in2, Value(SampleType::Int32));
    expect(idle(), calls(1, {count(0).noChanges()}));
}

// S1.CTX.11
TEST_F(MultiReader2Context, ConfigureDoesNotScheduleHandlerAfterFailedCall)
{
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    expect(configure(same()), ok().noChanges());
    expect(idle(), calls(0));
    expect(avail(), count(50));
}

// S1.CTX.12
TEST_F(MultiReader2Context, HandlerCanCallAvailableCountSkipAndReadWithDomain)
{
    setupHandler(
        [&]
        {
            avail();
            skip(50);
            readWithDomain(50);
            read(0, nullData);
        });
    blockWorker();
    write(in1, 100, 199);
    write(in2, 100, 199);
    unblockWorker();
    expect(idle(), calls(1, {count(100), count(50).at(100).noChanges(), count(50).at(150).noChanges(), count(0).noChanges()}));
}

// S1.CTX.13
TEST_F(MultiReader2Context, HandlerCanConfigureWithEqualParamsAndRead)
{
    setupHandler(
        [&]
        {
            configure(same());
            drain();
        });
    blockWorker();
    write(in1, 100, 199);
    write(in2, 100, 199);
    unblockWorker();
    expect(idle(), calls(1, {ok().noChanges(), count(100).at(100).noChanges(), count(0).noChanges()}));
}

// S1.CTX.14
TEST_F(MultiReader2Context, HandlerCanReconfigureWithChangedErrorPolicy)
{
    setupBase([&] { read(100); });
    handler(
        [&]
        {
            drain();
            configure(same().policy(Exclude));
        });
    blockWorker();
    write(in1, 100, 199);
    write(in2, 100, 199);
    unblockWorker();
    expect(idle(), calls(1, {count(100).at(100).noChanges(), count(0).noChanges(), ok().domainChanged().resync().allDescChanged()}));
}

// S1.CTX.15
TEST_F(MultiReader2Context, HandlerCanConstructAnotherReader)
{
    setupHandler(
        [&]
        {
            construct(same());
            drain();
        });
    blockWorker();
    write(in1, 100, 199);
    write(in2, 100, 199);
    unblockWorker();
    expect(idle(), calls(1, {ok(), count(100).at(100).noChanges(), count(0).noChanges()}));
}

// S1.CTX.17
TEST_F(MultiReader2Context, ReaderReleasedWhileHandlerCallIsScheduledIsDestroyed)
{
    given().ports();
    setupHandler([&] { drain(); });
    blockWorker();
    disconnect(in2);
    release();
    expectDestroyed();
    unblockWorker();
    expect(idle(), calls(0));
}

// S1.CTX.18
TEST_F(MultiReader2Context, HandlerReleasingReaderDestroysItAfterReturn)
{
    setupBase([&] { read(100); });
    handler(
        [&]
        {
            release();
            drain();
        });
    blockWorker();
    write(in1, 100, 199);
    write(in2, 100, 199);
    unblockWorker();
    expect(idle(), calls(1, {count(100).at(100).noChanges(), count(0).noChanges()}));
    expectDestroyed();
}

// S1.CTX.19
TEST_F(MultiReader2Context, ValueDescriptorChangeTriggersHandler)
{
    setupHandler([&] { drain(); });
    announce(in2, Value(SampleType::Int32));
    expect(idle(), calls(1, {count(0).descChanged(in2), count(0).noChanges()}));
}

// S1.CTX.20
TEST_F(MultiReader2Context, DomainDescriptorChangeTriggersHandler)
{
    setupHandler([&] { drain(); });
    announce(in1, Domain().start(3));
    expect(idle(), calls(1, {count(0).domainChanged().resync().allDescChanged(), count(0).noChanges()}));
}

// S1.CTX.21
TEST_F(MultiReader2Context, ValueAndDomainDescriptorChangeTriggersHandler)
{
    setupHandler([&] { drain(); });
    announce(in2, Value(SampleType::Int32), Domain().start(3));
    expect(idle(), calls(1, {count(0).descChanged(in2), count(0).noChanges()}));
}

// S1.CTX.22
TEST_F(MultiReader2Context, GapEventTriggersHandler)
{
    setupHandler([&] { drain(); });
    gap(in2);
    expect(idle(), calls(1, {count(0).noChanges()}));
}

// S1.CTX.23
TEST_F(MultiReader2Context, InputPortConnectTriggersHandler)
{
    given().ports().policy(Exclude).unconnected(in2);
    setup(
        [&]
        {
            handler([&] { drain(); });
            write(in1, 0, 99);
            idle();
        });
    connect(in2);
    expect(idle(), calls(1, {count(0).level(Ok).descChanged(in2), count(0).noChanges()}));
}

// S1.CTX.24
TEST_F(MultiReader2Context, InputPortDisconnectTriggersHandler)
{
    given().ports().policy(Exclude);
    setupHandler([&] { drain(); });
    disconnect(in2);
    expect(idle(), calls(1, {count(0).level(Warning), count(0).noChanges()}));
}

// S1.CTX.25
TEST_F(MultiReader2Context, DisconnectAndReconnectTriggerOneCallWithNoChanges)
{
    given().ports();
    setupHandler([&] { drain(); });
    blockWorker();
    disconnect(in2);
    connect(in2);
    unblockWorker();
    expect(idle(), calls(1, {count(0).noChanges()}));
}

// S1.CTX.26
TEST_F(MultiReader2Context, EqualDescriptorAndOtherEventDoNotTriggerHandler)
{
    setupHandler([&] { drain(); });
    announce(in2, Value::same());
    otherEvent(in2);
    expect(idle(), calls(0));
}

// S1.CTX.27
TEST_F(MultiReader2Context, NullValueDescriptorTriggersHandler)
{
    given().policy(Exclude);
    setupHandler([&] { drain(); });
    announce(in2, Value::null());
    expect(idle(), calls(1, {count(0).level(Warning), count(0).noChanges()}));
}

// S1.CTX.28
TEST_F(MultiReader2Context, NullDomainDescriptorTriggersHandler)
{
    given().policy(Exclude);
    setupHandler([&] { drain(); });
    announce(in2, Domain::null());
    expect(idle(), calls(1, {count(0).level(Warning), count(0).noChanges()}));
}

// S1.CTX.29
TEST_F(MultiReader2Context, SignalRemovalTriggersHandler)
{
    given().policy(Exclude);
    setupHandler([&] { drain(); });
    removeSignal(in2);
    expect(idle(), calls(1, {count(0).level(Warning), count(0).noChanges()}));
}

// S1.CTX.30
TEST_F(MultiReader2Context, HandlerLeavingValueDescriptorChangeUnreadIsCalledAgain)
{
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    announce(in2, Value(SampleType::Int32));
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).descChanged(in2)}));
}

// S1.CTX.31
TEST_F(MultiReader2Context, HandlerLeavingDomainChangeUnreadIsCalledAgain)
{
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    announce(in1, Domain().start(3));
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).domainChanged().resync().allDescChanged()}));
}

// S1.CTX.32
TEST_F(MultiReader2Context, HandlerLeavingValueAndDomainChangeUnreadIsCalledAgain)
{
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    announce(in2, Value(SampleType::Int32), Domain().start(3));
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).descChanged(in2)}));
}

// S1.CTX.33
TEST_F(MultiReader2Context, HandlerLeavingGapUnreadIsCalledAgain)
{
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    gap(in2);
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).noChanges()}));
}

// S1.CTX.34
TEST_F(MultiReader2Context, HandlerIsCalledAgainForLeftoverSamplesAfterConnect)
{
    given().ports().policy(Exclude).unconnected(in2);
    setup(
        [&]
        {
            handler([&] { read(150); });
            write(in1, 0, 99);
            idle();
        });
    connect(in2);
    expect(idle(), calls(1, {count(0).level(Ok).descChanged(in2)}));
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).noChanges()}));
}

// S1.CTX.35
TEST_F(MultiReader2Context, HandlerIsCalledAgainForLeftoverSamplesAfterDisconnect)
{
    given().ports().policy(Exclude);
    setupHandler([&] { read(150); });
    disconnect(in2);
    expect(idle(), calls(1, {count(0).level(Warning)}));
    blockWorker();
    write(in1, 100, 299);
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).noChanges()}));
}

// S1.CTX.36
TEST_F(MultiReader2Context, HandlerIsCalledAgainForLeftoverSamplesAfterReconnect)
{
    given().ports();
    setupHandler([&] { read(150); });
    blockWorker();
    disconnect(in2);
    connect(in2);
    unblockWorker();
    expect(idle(), calls(1, {count(0).noChanges()}));
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).noChanges()}));
}

// S1.CTX.37
TEST_F(MultiReader2Context, HandlerIsCalledAgainPastEqualDescriptorAndOtherEvent)
{
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    announce(in2, Value::same());
    otherEvent(in2);
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).noChanges()}));
}

// S1.CTX.38
TEST_F(MultiReader2Context, HandlerLeavingNullValueDescriptorUnreadIsCalledAgain)
{
    given().policy(Exclude);
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    announce(in2, Value::null());
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).level(Warning)}));
}

// S1.CTX.39
TEST_F(MultiReader2Context, HandlerLeavingNullDomainDescriptorUnreadIsCalledAgain)
{
    given().policy(Exclude);
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    announce(in2, Domain::null());
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).level(Warning)}));
}

// S1.CTX.40
TEST_F(MultiReader2Context, HandlerIsCalledAgainForLeftoverSamplesAfterSignalRemoval)
{
    given().policy(Exclude);
    setupHandler([&] { read(150); });
    removeSignal(in2);
    expect(idle(), calls(1, {count(0).level(Warning)}));
    blockWorker();
    write(in1, 100, 299);
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(50).at(250).noChanges()}));
}

// S1.CTX.41
TEST_F(MultiReader2Context, ValueDescriptorChangeCallsHandlerAgainAfterThrow)
{
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    announce(in2, Value(SampleType::Int32));
    expect(idle(), calls(1, {count(50).at(250).descChanged(in2)}));
}

// S1.CTX.42
TEST_F(MultiReader2Context, DomainChangeCallsHandlerAgainAfterThrow)
{
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    announce(in1, Domain().start(3));
    expect(idle(), calls(1, {count(50).at(250).domainChanged().resync().allDescChanged()}));
}

// S1.CTX.43
TEST_F(MultiReader2Context, ValueAndDomainChangeCallsHandlerAgainAfterThrow)
{
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    announce(in2, Value(SampleType::Int32), Domain().start(3));
    expect(idle(), calls(1, {count(50).at(250).descChanged(in2)}));
}

// S1.CTX.44
TEST_F(MultiReader2Context, GapCallsHandlerAgainAfterThrow)
{
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    gap(in2);
    expect(idle(), calls(1, {count(50).at(250).noChanges()}));
}

// S1.CTX.45
TEST_F(MultiReader2Context, ConnectCallsHandlerAgainAfterThrow)
{
    given().ports().policy(Exclude).unconnected(in2);
    setup(
        [&]
        {
            handler(
                [&]
                {
                    read(150);
                    throw std::runtime_error("handler throws");
                });
            write(in1, 0, 99);
            idle();
            read(150);
        });
    blockWorker();
    write(in1, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    connect(in2);
    expect(idle(), calls(1, {count(50).at(250).level(Ok).descChanged(in2)}));
}

// S1.CTX.46
TEST_F(MultiReader2Context, DisconnectCallsHandlerAgainAfterThrow)
{
    given().ports().policy(Exclude);
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    disconnect(in2);
    expect(idle(), calls(1, {count(50).at(250).level(Warning)}));
}

// S1.CTX.47
TEST_F(MultiReader2Context, DisconnectAndReconnectCallsHandlerAgainAfterThrow)
{
    given().ports();
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    blockWorker();
    disconnect(in2);
    connect(in2);
    unblockWorker();
    expect(idle(), calls(1, {count(50).at(250).noChanges()}));
}

// S1.CTX.48
TEST_F(MultiReader2Context, EqualDescriptorAndOtherEventDoNotCallHandlerAfterThrow)
{
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    announce(in2, Value::same());
    otherEvent(in2);
    expect(idle(), calls(0));
    expect(avail(), count(50));
}

// S1.CTX.49
TEST_F(MultiReader2Context, NullValueDescriptorCallsHandlerAgainAfterThrow)
{
    given().policy(Exclude);
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    announce(in2, Value::null());
    expect(idle(), calls(1, {count(50).at(250).level(Warning)}));
}

// S1.CTX.50
TEST_F(MultiReader2Context, NullDomainDescriptorCallsHandlerAgainAfterThrow)
{
    given().policy(Exclude);
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    announce(in2, Domain::null());
    expect(idle(), calls(1, {count(50).at(250).level(Warning)}));
}

// S1.CTX.51
TEST_F(MultiReader2Context, SignalRemovalCallsHandlerAgainAfterThrow)
{
    given().policy(Exclude);
    setupHandler(
        [&]
        {
            read(150);
            throw std::runtime_error("handler throws");
        },
        [&] { read(150); });
    blockWorker();
    write(in1, 100, 299);
    write(in2, 100, 299);
    unblockWorker();
    expect(idle(), calls(1, {count(150).at(100).noChanges()}));
    removeSignal(in2);
    expect(idle(), calls(1, {count(50).at(250).level(Warning)}));
}

// S1.CTX.52
TEST_F(MultiReader2Context, DisconnectReadFromUserThreadStillCallsHandlerOnce)
{
    given().ports().policy(Exclude);
    setupBase();
    handler([&] { avail(); });
    expect(idle(), calls(0));
    blockWorker();
    disconnect(in2);
    expect(read(100), count(100).at(0).level(Warning));
    unblockWorker();
    expect(idle(), calls(1, {count(0)}));
}

// S1.CTX.53
TEST_F(MultiReader2Context, FirstHandlerCallWaitsForAllInputsAndGetsFirstStatus)
{
    handler([&] { drain(); });
    write(in1, 0, 9);
    expect(idle(), calls(0));
    write(in2, 0, 9);
    expect(idle(),
           calls(1, {count(0).level(Ok).domainChanged().resync().allDescChanged(), count(10).at(0).noChanges(), count(0).noChanges()}));
}

// S1.CTX.54
TEST_F(MultiReader2Context, DescriptorChangeBeforeFirstDataCallsHandlerWithFirstStatus)
{
    handler([&] { drain(); });
    announce(in2, Value(SampleType::Int32));
    expect(idle(), calls(1, {count(0).level(Ok).domainChanged().resync().allDescChanged(), count(0).noChanges()}));
}

// S1.CTX.55
TEST_F(MultiReader2Context, HandlerReadingFirstStatusIsCalledAgainForData)
{
    handler([&] { read(150); });
    blockWorker();
    write(in1, 0, 99);
    write(in2, 0, 99);
    unblockWorker();
    expect(idle(), calls(2, {count(0).level(Ok).domainChanged().resync().allDescChanged(), count(100).at(0).noChanges()}));
}

// S1.CTX.56
TEST_F(MultiReader2Context, HandlerIsCalledAgainForEventBehindTooFewSamples)
{
    given().minReadCount(10);
    setupHandler([&] { read(150); });
    blockWorker();
    write(in1, 100, 254);
    write(in2, 100, 254);
    announce(in2, Value(SampleType::Int32));
    unblockWorker();
    expect(idle(), calls(2, {count(150).at(100).noChanges(), count(0).resync().descChanged(in2)}));
}

}
