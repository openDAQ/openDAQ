// Suite 3: concurrent runs, their replay against a serial reference, and the handler controls they use

#include "multi_reader2_harness.h"
#include "multi_reader2_harness_state.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <random>
#include <thread>

namespace daq::mr2test
{

using namespace detail;

// ---------------------------------------------------------------------------------------------------------------------
// Suite 3

SizeT MultiReader2Test::handlerCallsStarted()
{
    return handlerState->started;
}

void MultiReader2Test::waitUntilHandlerSaw(Input in, InputError error)
{
    const bool seen = waitForHandler(
        [&]
        {
            std::scoped_lock lock(handlerState->mutex);
            for (const auto& r : handlerState->log.results)
                if (r.status.assigned())
                    for (const auto& i : r.status.getInputs())
                        if (slotOf(i.getInput()) == in.slot && i.getError() == error)
                            return true;
            return false;
        });
    EXPECT_TRUE(seen) << "the handler saw in" << in.slot << " in error";
}

bool MultiReader2Test::waitForHandler(const std::function<bool()>& condition)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (handlerState->subscribed && std::chrono::steady_clock::now() < deadline)
    {
        if (condition())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return condition();
}

bool MultiReader2Test::handlerRunning()
{
    return handlerState->running > 0;
}

void MultiReader2Test::holdNextHandlerCall()
{
    handlerState->holdNext = true;
}

void MultiReader2Test::waitForHeldHandlerCall()
{
    std::unique_lock lock(handlerState->mutex);
    if (!handlerState->heldCv.wait_for(lock, std::chrono::seconds(5), [this] { return handlerState->held; }))
        ADD_FAILURE() << "no handler call parked within 5 s";
}

void MultiReader2Test::releaseHandlerCall()
{
    {
        std::scoped_lock lock(handlerState->mutex);
        handlerState->held = false;
    }
    handlerState->heldCv.notify_all();
}

Script MultiReader2Test::eachInputWrites(int64_t first, int64_t last)
{
    reader();
    Script s;
    for (int slot : slotsInForce())
        s.push_back({Input{slot}, sendWrite(Input{slot}, first, last)});
    return s;
}

int64_t MultiReader2Test::random(int64_t low, int64_t high)
{
    if (!rng)
    {
        // MR2_SEED replays a recorded run; otherwise gtest's seed when one is set (--gtest_random_seed, --gtest_shuffle)
        const char* fixed = std::getenv("MR2_SEED");
        const auto gtestSeed = static_cast<uint64_t>(testing::UnitTest::GetInstance()->random_seed());
        const uint64_t seed = fixed ? std::stoull(fixed) : gtestSeed != 0 ? gtestSeed : std::random_device{}();
        RecordProperty("seed", std::to_string(seed));
        recordedSeed = seed;
        rng.emplace(seed);
    }
    return std::uniform_int_distribution<int64_t>(low, high)(*rng);
}

void MultiReader2Test::pause(int64_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void MultiReader2Test::restart(SizeT runWorkers)
{
    stopEndlessWriters();
    if (workerHeld)
        releaseWorker();
    if (handlerState)
        releaseHandlerCall();
    readerObject.release();
    otherReaders.clear();
    holders.clear();
    if (scheduler.assigned())
    {
        scheduler.waitAll();
        scheduler.stop();
    }
    inputs.clear();
    spares.clear();
    handlerState = std::make_unique<Handler>();
    {
        std::scoped_lock lock(stub->mutex);
        stub->asked.clear();
        stubSaw.clear();
    }
    {
        std::scoped_lock lock(owner->mutex);
        owner->cache.clear();
    }
    constructed = false;
    readerRef = nullptr;
    readerListener = nullptr;
    paramsInForce = nullptr;
    paramsObject = nullptr;
    previousStatus = nullptr;
    lastResult = Result{Result::Kind::Avail};
    scheduler = nullptr;
    context = nullptr;
    contextWithoutScheduler = nullptr;
    workers = runWorkers;
    errorsAtIdle = errorsBeforeIdle = countingSink(errorSink).count();
}

CaseRun MultiReader2Test::runCase(SizeT runWorkers, bool concurrent, const std::function<void(bool)>& body)
{
    restart(runWorkers);
    Watchdog watchdog(recordedSeed);
    statusInvariants = false;
    muted = true;
    CaseRun run;
    {
        std::scoped_lock lock(runMutex);
        runLog.clear();
    }
    recordingRun = true;
    body(concurrent);
    flush();
    idle();
    if (reader().assigned())
    {
        run.availError = reader()->getAvailableCount(&run.availAfterIdle);
        daqClearErrorInfo();
        const auto final = drain();
        if (!final.empty())
            run.finalRead = final.front();
    }
    recordingRun = false;
    {
        std::scoped_lock lock(runMutex);
        run.reads = std::move(runLog);
        runLog.clear();
    }
    for (const auto& r : run.reads)
        if (r.status.assigned())
            run.last = r.status;
    run.maxOverlap = handlerState->maxRunning;
    run.values = delivered(run.reads, paramsInForce.assigned() ? paramsInForce.getValueReadType() : SampleType::Float64, false, false);
    muted = false;
    return run;
}

void MultiReader2Test::replay(Timeline tl, unsigned invariants)
{
    referenceDelivers = tl != Timeline::Apart && tl != Timeline::Starve1 && tl != Timeline::StarveAll && tl != Timeline::StarveMain &&
                        tl != Timeline::Short;
    replay([this, tl] { return script(tl); }, invariants);
}

void MultiReader2Test::replay(const std::function<Script()>& s, unsigned invariants)
{
    replay(
        [this, s](bool concurrent)
        {
            handler([this] { drain(); });
            runScript(s(), concurrent);
        },
        invariants);
}

void MultiReader2Test::replay(const std::function<void(bool)>& body, unsigned invariants)
{
    const CaseRun reference = runCase(1, false, body);
    const CaseRun run = runCase(fixtureWorkers, true, body);
    checkReplay(reference, run, invariants);
}

void MultiReader2Test::runConcurrently(const std::function<void(bool)>& body, unsigned invariants)
{
    const CaseRun run = runCase(fixtureWorkers, true, body);
    checkReplay(run, run, invariants & ~(INV4 | INV5 | INV9));
}

// The samples each input delivered, in read order. INV1 holds on the way: values rise except on the first delivering
// read after a status with resync. INV10, where values are ticks: a read delivers the same ticks on every input, which
// holds however the ports' packets interleave
std::map<int, std::vector<double>> MultiReader2Test::delivered(const std::vector<Result>& reads,
                                                               SampleType readType,
                                                               bool checkInv1,
                                                               bool checkInv10)
{
    checkInv10 = checkInv10 && config->values == Config::Values::Tick;
    std::map<int, std::vector<double>> values;
    std::map<int, double> lastValue;
    bool resyncPending = false;
    for (const auto& r : reads)
    {
        const bool copies = r.kind != Result::Kind::Skip && !r.args.nullData && OPENDAQ_SUCCEEDED(r.error);
        if (copies && r.count > 0 && r.previous.assigned() && r.previous.getStatus() != Error)
        {
            if (resyncPending)
                lastValue.clear();
            resyncPending = false;
            std::optional<std::vector<double>> aligned;
            for (SizeT b = 0; b < r.buffers.size(); ++b)
            {
                const int slot = r.slots[b];
                MultiReader2InputStatusPtr prevInput;
                for (const auto& i : r.previous.getInputs())
                    if (slotOf(i.getInput()) == slot && i.getError() == InputError::None)
                        prevInput = i;
                if (!prevInput.assigned())
                    continue;
                const auto desc = prevInput.getDescriptor();
                const SampleType type = readType == SampleType::Undefined ? desc.getSampleType() : readType;
                const SizeT size = elementCount(desc) * typeSize(type);
                std::vector<double> got;
                for (SizeT k = 0; k < r.count && (k + 1) * size <= r.buffers[b].size(); ++k)
                {
                    const double v = readElement(r.buffers[b].data() + k * size, type);
                    const auto last = lastValue.find(slot);
                    if (checkInv1 && last != lastValue.end())
                        EXPECT_GT(v, last->second) << "INV1: in" << slot << "'s samples arrive once and in tick order";
                    lastValue[slot] = v;
                    values[slot].push_back(v);
                    got.push_back(v);
                }
                if (checkInv10 && aligned)
                    EXPECT_EQ(got, *aligned) << "INV10: in" << slot << " delivers the ticks of the read's other inputs";
                else if (checkInv10)
                    aligned = std::move(got);
            }
        }
        if (r.status.assigned() && r.status.getResynchronized())
            resyncPending = true;
    }
    return values;
}

namespace
{
int64_t firstTickOf(const Result& r, int64_t start)
{
    if (r.kind == Result::Kind::ReadWithDomain)
    {
        int64_t tick;
        std::memcpy(&tick, r.domainBuffer.data(), sizeof tick);
        return tick;
    }
    return static_cast<int64_t>(r.packetOffset) + start;
}

struct FinalState
{
    ComponentStatus level;
    std::string message;
    int main;
    DataDescriptorPtr domain;
    std::vector<std::tuple<int, InputError, DataDescriptorPtr, SizeT>> inputs;
};
}

void MultiReader2Test::checkReplay(const CaseRun& reference, const CaseRun& run, unsigned inv)
{
    const SampleType readType = paramsInForce.assigned() ? paramsInForce.getValueReadType() : SampleType::Float64;
    for (const auto& r : run.reads)
        if (OPENDAQ_FAILED(r.error))
        {
            ADD_FAILURE() << "a reading call of the run failed: " << r.step << " returned " << errorName(r.error);
            break;
        }
    const auto values = delivered(run.reads, readType, inv & INV1, inv & INV10);

    if (inv & INV2)
    {
        bool resyncPending = true;
        int64_t next = 0;
        for (const auto& r : run.reads)
        {
            if (OPENDAQ_SUCCEEDED(r.error) && r.count > 0)
            {
                int64_t start = 0, delta = 1;
                if (r.previous.assigned() && r.previous.getDomainDescriptor().assigned())
                    if (const auto d = TickDomain::from(r.previous.getDomainDescriptor()))
                    {
                        start = d->start;
                        delta = d->delta;
                    }
                const int64_t first = firstTickOf(r, start);
                if (!resyncPending)
                    EXPECT_EQ(first, next) << "INV2: packetOffset continues the previous read";
                resyncPending = false;
                next = first + static_cast<int64_t>(r.count) * delta;
            }
            if (r.status.assigned() && r.status.getResynchronized())
                resyncPending = true;
        }
    }

    if ((inv & INV3) && run.last.assigned() && run.last.getStatus() != Error)
    {
        for (const auto& i : run.last.getInputs())
        {
            const int slot = slotOf(i.getInput());
            if (i.getError() == InputError::None && input(slot).signal)
                EXPECT_TRUE(BaseObjectPtr::Equals(i.getDescriptor(), input(slot).signal->valueDesc))
                    << "INV3: in" << slot << "'s descriptor is the last one written";
        }
        const int mainSlot = slotOf(run.last.getMainInput());
        if (mainSlot > 0 && input(mainSlot).signal)
            EXPECT_TRUE(BaseObjectPtr::Equals(run.last.getDomainDescriptor(), input(mainSlot).signal->domainDesc))
                << "INV3: the main domain descriptor is the last one written";
    }

    if (inv & INV4)
    {
        EXPECT_EQ(run.availError, OPENDAQ_SUCCESS) << "INV4: avail after idle returned " << errorName(run.availError);
        EXPECT_EQ(run.availAfterIdle, 0u) << "INV4: nothing is left to read once idle returns";
        ASSERT_TRUE(run.finalRead.has_value()) << "INV4: the final drain read";
        EXPECT_EQ(run.finalRead->count, 0u) << "INV4: the final drain's first read returns 0";
        if (run.finalRead->status.assigned() && reference.finalRead && reference.finalRead->status.assigned())
            EXPECT_EQ(run.finalRead->status.getHasChanges(), reference.finalRead->status.getHasChanges())
                << "INV4: the final drain's first read has changes as in the reference run";
    }

    if (inv & INV5)
    {
        const auto stateOf = [this](const MultiReader2StatusPtr& s)
        {
            FinalState st{s.getStatus(), s.getStatusMessage().toStdString(), slotOf(s.getMainInput()), s.getDomainDescriptor(), {}};
            for (const auto& i : s.getInputs())
                st.inputs.emplace_back(slotOf(i.getInput()), i.getError(), i.getDescriptor(), i.getSampleSize());
            return st;
        };
        ASSERT_TRUE(run.last.assigned() && reference.last.assigned()) << "INV5: both runs end with a status";
        const auto a = stateOf(run.last);
        // the reference ran on its own inputs, which map to the same slots by local ID
        FinalState b{
            reference.last.getStatus(), reference.last.getStatusMessage().toStdString(), 0, reference.last.getDomainDescriptor(), {}};
        b.main =
            reference.last.getMainInput().assigned() ? std::stoi(reference.last.getMainInput().getLocalId().toStdString().substr(2)) : 0;
        for (const auto& i : reference.last.getInputs())
            b.inputs.emplace_back(
                std::stoi(i.getInput().getLocalId().toStdString().substr(2)), i.getError(), i.getDescriptor(), i.getSampleSize());
        EXPECT_EQ(a.level, b.level) << "INV5: level";
        EXPECT_EQ(a.message, b.message) << "INV5: message";
        EXPECT_EQ(a.main, b.main) << "INV5: main input";
        EXPECT_TRUE(BaseObjectPtr::Equals(a.domain, b.domain)) << "INV5: domain descriptor";
        ASSERT_EQ(a.inputs.size(), b.inputs.size()) << "INV5: inputs";
        for (SizeT k = 0; k < a.inputs.size(); ++k)
        {
            EXPECT_EQ(std::get<0>(a.inputs[k]), std::get<0>(b.inputs[k])) << "INV5: input slot";
            EXPECT_EQ(std::get<1>(a.inputs[k]), std::get<1>(b.inputs[k])) << "INV5: error of in" << std::get<0>(a.inputs[k]);
            EXPECT_TRUE(BaseObjectPtr::Equals(std::get<2>(a.inputs[k]), std::get<2>(b.inputs[k])))
                << "INV5: descriptor of in" << std::get<0>(a.inputs[k]);
            EXPECT_EQ(std::get<3>(a.inputs[k]), std::get<3>(b.inputs[k])) << "INV5: sample size of in" << std::get<0>(a.inputs[k]);
        }
    }

    if (inv & INV8)
        EXPECT_LE(run.maxOverlap, 1) << "INV8: handler calls never overlap";

    if (inv & INV9)
    {
        // The reference run's inputs are gone by now, so its samples were taken when it ended
        EXPECT_EQ(values, reference.values) << "INV9: each input delivers the samples of the reference run";
        // A reference that delivered nothing would make the comparison above pass on any run
        if (referenceDelivers && reference.last.assigned())
            for (const auto& i : reference.last.getInputs())
                if (i.getError() == InputError::None)
                {
                    const int slot = std::stoi(i.getInput().getLocalId().toStdString().substr(2));
                    const auto it = reference.values.find(slot);
                    EXPECT_TRUE(it != reference.values.end() && !it->second.empty())
                        << "INV9: the reference run delivered nothing on in" << slot;
                }
    }
}

void MultiReader2Test::expectReleasedCleanly(SizeT callsAtRelease, SizeT inFlight)
{
    flush();
    EXPECT_EQ(handlerState->startedWithoutReader.load(), 0u) << "INV6: no handler call runs after the reader was destroyed";
    // A call that took the reader before the release is counted when it enters the body, after the release (R-THR-2)
    EXPECT_LE(handlerCallsStarted(), callsAtRelease + inFlight)
        << "INV6: no handler call starts after the last release but those in flight";
    expectDestroyed();
    for (const auto& [slot, in] : inputs)
        if (in->port.assigned())
        {
            const auto state = portState(slot);
            EXPECT_EQ(state.first, Listener::None) << "INV6: in" << slot << " has no listener";
            EXPECT_EQ(state.second, PacketReadyNotification::None) << "INV6: in" << slot << " has notification method None";
        }
        else  // the port the reader created for a signal input is disconnected with the reader
            EXPECT_EQ(in->own->value.getConnections().getCount(), 0u) << "INV6: in" << slot << "'s signal has no connection left";
}

}
