#include "multi_reader2_harness.h"

namespace daq::mr2test
{

// Suite 3: four scheduler workers, one writer thread per input; each case runs against its reference under DATA.WRITE.1
class MultiReader2Concurrency : public MultiReader2Test
{
public:
    MultiReader2Concurrency()
        : MultiReader2Test(4)
    {
    }
};

constexpr unsigned AllButInv6 = INV1 | INV2 | INV3 | INV4 | INV5 | INV7 | INV8 | INV9 | INV10;
constexpr unsigned AllButInv6AndInv9 = INV1 | INV2 | INV3 | INV4 | INV5 | INV7 | INV8 | INV10;

// S3.TL.01
TEST_F(MultiReader2Concurrency, AlignedInputsDeliverEverySampleOnce)
{
    replay(Timeline::Base, AllButInv6);
}

// S3.TL.02
TEST_F(MultiReader2Concurrency, InputsStartingApartSynchronizeWithoutLoss)
{
    replay(Timeline::Apart, AllButInv6);
}

// S3.TL.03
TEST_F(MultiReader2Concurrency, NonMainInputAheadBeyondSyncLimitFailsSync)
{
    given().policy(Exclude);
    replay(Timeline::Ahead, AllButInv6AndInv9);
}

// S3.TL.04
TEST_F(MultiReader2Concurrency, NonMainInputBehindBeyondSyncLimitFailsSync)
{
    given().policy(Exclude);
    replay(Timeline::Behind, AllButInv6AndInv9);
}

// S3.TL.05
TEST_F(MultiReader2Concurrency, ForwardTimestampJumpOnOneInputResynchronizes)
{
    replay(Timeline::JumpAhead, AllButInv6);
}

// S3.TL.06
TEST_F(MultiReader2Concurrency, BackwardTimestampJumpOnOneInputResynchronizes)
{
    replay(Timeline::JumpBack, AllButInv6);
}

// S3.TL.07
TEST_F(MultiReader2Concurrency, OneSilentInputAmongManyKeepsReaderSynchronizing)
{
    given().inputs(10);
    replay(Timeline::Starve1, AllButInv6);
}

// S3.TL.08
TEST_F(MultiReader2Concurrency, SilentNonMainInputsKeepReaderSynchronizing)
{
    replay(Timeline::StarveAll, AllButInv6);
}

// S3.TL.09
TEST_F(MultiReader2Concurrency, SilentMainInputKeepsReaderSynchronizing)
{
    replay(Timeline::StarveMain, AllButInv6);
}

// S3.TL.10
TEST_F(MultiReader2Concurrency, FewerSamplesThanMinReadCountStayUnread)
{
    given().minReadCount(10);
    replay(Timeline::Short, AllButInv6);
}

// S3.TL.11
TEST_F(MultiReader2Concurrency, SilentInputFailsSyncOnceMainPassesSyncLimit)
{
    given().policy(Exclude);
    replay(Timeline::Starve2s, AllButInv6AndInv9);
}

// S3.TL.12
TEST_F(MultiReader2Concurrency, SyncFailedInputAheadRejoinsOnceWithinLimit)
{
    given().policy(Exclude);
    replay(
        [&](bool concurrent)
        {
            handler([&] { drain(); });
            runScript(script(Timeline::Ahead), concurrent);
            waitUntilHandlerSaw(in2, InputError::SyncFailed);
            // TL.CATCHUP with the main input's first step written alone: a late input keeps no data until it is within
            // the limit (decision 31), so in2 packets that raced ahead of in1 100..299 would be dropped, and nothing
            // written after them would bring in2 back
            runScript({{in1, sendWrite(in1, 100, 299)}}, concurrent);
            idle();  // the writer's return does not mean in1 299 was ingested; ingest runs on the scheduler
            runScript({{in2, sendWrite(in2, 2200, 2299)}, {in1, sendWrite(in1, 300, 2299)}}, concurrent);
        },
        AllButInv6AndInv9);
}

// S3.TL.13
TEST_F(MultiReader2Concurrency, SyncFailedInputBehindRejoinsOnceWithinLimit)
{
    given().policy(Exclude);
    replay(
        [&](bool concurrent)
        {
            handler([&] { drain(); });
            runScript(script(Timeline::Behind), concurrent);
            waitUntilHandlerSaw(in2, InputError::SyncFailed);
            runScript(script(Timeline::CatchUp), concurrent);
        },
        AllButInv6AndInv9);
}

// S3.TL.15
TEST_F(MultiReader2Concurrency, NanosecondEpochTimestampsSynchronize)
{
    given().valuesAsIndex().signal(in1, sig::epochNs()).signal(in2, sig::epochNs());
    replay(Timeline::Epoch, AllButInv6);
}

// S3.TL.16
TEST_F(MultiReader2Concurrency, MicrosecondAndNanosecondEpochTimestampsSynchronize)
{
    given().valuesAsIndex().signal(in1, sig::epochUs()).signal(in2, sig::epochNs());
    replay(Timeline::Epoch, AllButInv6);
}

// S3.TL.17
TEST_F(MultiReader2Concurrency, EpochTimestampsWithNonSimplifiedResolutionSynchronize)
{
    given().valuesAsIndex().signal(in1, sig::epochNs()).signal(in2, sig::epochNonSimplified());
    replay(Timeline::Epoch, AllButInv6);
}

// S3.EVT.01
TEST_F(MultiReader2Concurrency, ValueTypeChangeMidStreamOnNonMainInput)
{
    replay([&] { return eventScript(in2, {sendAnnounce(in2, Value(SampleType::Int32), std::nullopt)}); }, AllButInv6);
}

// S3.EVT.02
TEST_F(MultiReader2Concurrency, DomainStartChangeMidStreamOnMainInput)
{
    replay([&] { return eventScript(in1, {sendAnnounce(in1, std::nullopt, Domain().start(3))}); }, AllButInv6AndInv9);
}

// S3.EVT.03
TEST_F(MultiReader2Concurrency, DomainStartChangeMidStreamOnNonMainInput)
{
    replay([&] { return eventScript(in2, {sendAnnounce(in2, std::nullopt, Domain().start(3))}); }, AllButInv6);
}

// S3.EVT.04
TEST_F(MultiReader2Concurrency, ValueAndDomainChangeMidStreamOnNonMainInput)
{
    replay([&] { return eventScript(in2, {sendAnnounce(in2, Value(SampleType::Int32), Domain().start(3))}); }, AllButInv6);
}

// S3.EVT.05
TEST_F(MultiReader2Concurrency, GapMidStreamOnNonMainInput)
{
    replay([&] { return eventScript(in2, {sendGap(in2)}, 300, 349); }, AllButInv6);
}

// S3.EVT.06
TEST_F(MultiReader2Concurrency, NonMainPortConnectedBeforeItsFirstData)
{
    given().ports().unconnected(in2).policy(Exclude);
    replay([&] { return Script{{in1, sendWrite(in1, 0, 99)}, {in2, sendConnect(in2)}, {in2, sendWrite(in2, 0, 99)}}; }, AllButInv6AndInv9);
}

// S3.EVT.07
TEST_F(MultiReader2Concurrency, DisconnectMidStreamOnNonMainPort)
{
    given().ports().policy(Exclude);
    replay([&] { return eventScript(in2, {sendDisconnect(in2)}, NoRest); }, AllButInv6AndInv9);
}

// S3.EVT.08
TEST_F(MultiReader2Concurrency, DisconnectAndReconnectMidStreamOnNonMainPort)
{
    given().ports();
    replay([&] { return eventScript(in2, {sendDisconnect(in2), sendConnect(in2)}); }, AllButInv6AndInv9);
}

// S3.EVT.09
TEST_F(MultiReader2Concurrency, EqualValueDescriptorResentMidStreamOnNonMainInput)
{
    replay([&] { return eventScript(in2, {sendAnnounce(in2, Value::same(), std::nullopt)}); }, AllButInv6);
}

// S3.EVT.10
TEST_F(MultiReader2Concurrency, NullValueDescriptorMidStreamOnNonMainInput)
{
    given().policy(Exclude);
    replay([&] { return eventScript(in2, {sendAnnounce(in2, Value::null(), std::nullopt)}); }, AllButInv6AndInv9);
}

// S3.EVT.11
TEST_F(MultiReader2Concurrency, NullDomainDescriptorMidStreamOnNonMainInput)
{
    given().policy(Exclude);
    replay([&] { return eventScript(in2, {sendAnnounce(in2, std::nullopt, Domain::null())}); }, AllButInv6AndInv9);
}

// S3.EVT.12
TEST_F(MultiReader2Concurrency, SignalRemovedMidStreamOnNonMainInput)
{
    given().policy(Exclude);
    replay([&] { return eventScript(in2, {sendRemoval(in2)}, NoRest); }, AllButInv6AndInv9);
}

// S3.EVT.13
TEST_F(MultiReader2Concurrency, UnrelatedEventPacketMidStreamOnNonMainInput)
{
    replay([&] { return eventScript(in2, {sendOtherEvent(in2)}); }, AllButInv6);
}

// S3.LIFE.01
TEST_F(MultiReader2Concurrency, LongIngestWithReaderAliveDeliversEverySampleOnce)
{
    replay(
        [&](bool concurrent)
        {
            handler([&] { drain(); });
            // Chunks of 1000, each ingested before the next, keep the writers within the 2000-tick sync limit
            for (int64_t first = 0; first < 10000; first += 1000)
            {
                runScript(eachInputWrites(first, first + 999), concurrent);
                idle();
            }
        },
        INV1 | INV2 | INV4 | INV7 | INV8 | INV9 | INV10);
    const SizeT calls = handlerCallsStarted();
    release();
    expectReleasedCleanly(calls, 0);  // idle returned and nothing writes: no call is in flight
}

// S3.LIFE.02
TEST_F(MultiReader2Concurrency, ReleaseWhileHandlerCallIsScheduledDestroysReaderSafely)
{
    given().ports();
    SizeT callsAtRelease = 0;
    runConcurrently(
        [&](bool)
        {
            handler([&] { drain(); });
            holdNextHandlerCall();  // the first call parks with the reader, so the release always finds a call running
            startEndlessWriters();
            waitForHeldHandlerCall();
            callsAtRelease = handlerCallsStarted();
            release();
            releaseHandlerCall();
            pause(50);
            stopEndlessWriters();
        },
        INV7 | INV8);
    expectReleasedCleanly(callsAtRelease, 1);  // the parked call, which took the reader before the release
}

// S3.LIFE.03
TEST_F(MultiReader2Concurrency, ReleaseInsideHandlerDestroysReaderSafely)
{
    given().ports();
    const SizeT releaseOn = static_cast<SizeT>(random(1, 50));
    std::atomic<bool> released{false};
    runConcurrently(
        [&](bool)
        {
            handler(
                [&]
                {
                    read(10);
                    if (handlerCallsStarted() == releaseOn)
                    {
                        release();
                        released = true;
                    }
                });
            startEndlessWriters();
            EXPECT_TRUE(waitForHandler([&] { return released.load(); })) << "the handler released the reader on call " << releaseOn;
            pause(50);
            stopEndlessWriters();
        },
        INV7 | INV8);
    expectReleasedCleanly(releaseOn, 0);  // the releasing call holds the reader, and calls never overlap
}

// S3.RACE.01
TEST_F(MultiReader2Concurrency, IngestWhileHandlerLeavesDataPendingLosesNoTrigger)
{
    given().onePerPacket();
    replay(
        [&](bool concurrent)
        {
            handler([&] { read(1); });
            runScript(eachInputWrites(0, 999), concurrent);
        },
        INV1 | INV2 | INV4 | INV5 | INV7 | INV8 | INV9 | INV10);
}

// S3.RACE.02
TEST_F(MultiReader2Concurrency, TenPortsWrittenInParallelDeliverEverySampleOnce)
{
    given().ports().inputs(10);
    replay([&] { return eachInputWrites(0, 999); }, AllButInv6);
}

// S3.RACE.03
TEST_F(MultiReader2Concurrency, LoopReadingOnUserThreadDuringIngestLosesNoSamples)
{
    replay(
        [&](bool concurrent)
        {
            if (concurrent)
                startScript(eachInputWrites(0, 999));
            else
                runScript(eachInputWrites(0, 999), false);
            // Ingest runs behind the writers on the scheduler, so the loop ends only after the writers finished and
            // their packets were ingested
            bool ingested = false;
            for (;;)
            {
                const bool writing = scriptRunning();
                const Result r = read(7);
                if (OPENDAQ_FAILED(r.error) || !r.status.assigned())
                    break;
                if (!writing && r.count == 0 && !r.status.getHasChanges())
                {
                    if (ingested)
                        break;
                    idle();
                    ingested = true;
                }
            }
            joinScript();
        },
        INV1 | INV2 | INV3 | INV4 | INV5 | INV7 | INV9 | INV10);
}

// S3.RACE.04
TEST_F(MultiReader2Concurrency, ConfigureFromHandlerDuringIngestLosesNoSamples)
{
    replay(
        [&](bool concurrent)
        {
            handler(
                [&]
                {
                    read(10);
                    if (handlerCallsStarted() % 5 == 0)
                        configure(same());
                });
            runScript(eachInputWrites(0, 999), concurrent);
        },
        INV1 | INV2 | INV4 | INV7 | INV8 | INV9 | INV10);
}

// S3.RACE.05
TEST_F(MultiReader2Concurrency, ThrowingHandlerDuringIngestLogsAndKeepsReading)
{
    replay(
        [&](bool concurrent)
        {
            handler(
                [&]
                {
                    if (handlerCallsStarted() % 3 == 0)
                        throw std::runtime_error("handler throws");
                    drain();
                });
            runScript(eachInputWrites(0, 999), concurrent);
            idle();
            expectLoggedErrors(handlerCallsStarted() / 3);
            handler([&] { drain(); });
            write(in1, 1000, 1009);
            write(in2, 1000, 1009);
            idle();
        },
        INV1 | INV2 | INV4 | INV7 | INV8 | INV9 | INV10);
}

// S3.RACE.06
TEST_F(MultiReader2Concurrency, ConfigureRemovingPortWhileItsWriterWritesDetachesPort)
{
    given().ports();
    runConcurrently(
        [&](bool)
        {
            handler([&] { drain(); });
            startEndlessWriters();
            pause(random(0, 50));
            configure(same().remove(in2));
            pause(50);
            stopEndlessWriters();
            idle();
            expectPort(in2, Listener::None, PacketReadyNotification::None);
        },
        INV7 | INV8);
}

}
