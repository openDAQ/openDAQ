/*
 * MultiReader2 under interleaved events: many inputs, each mixing data with descriptor changes, gaps, foreign
 * events, broken descriptors and disconnects in seeded random orders. Every read is checked against the
 * invariants of the status model and the alignment contract; the same seed has to produce the same trace.
 */
#include "test_multi_reader2_common.h"

#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>

#include <cmath>
#include <random>
#include <sstream>

using namespace daq;
using namespace testing;

namespace multi_reader2_stress_test
{
struct Scenario
{
    const char* name;
    unsigned seed;
    MultiReader2ErrorPolicy policy;
    bool overPorts;
    int inputs;
    int rounds;
};

inline std::ostream& operator<<(std::ostream& os, const Scenario& s)
{
    return os << s.name;
}

// The common clock is milliseconds since the base origin; every input's own ticks map onto it
constexpr std::int64_t BASE_SECONDS = 123;  // 00:02:03
}

using multi_reader2_stress_test::Scenario;

class MultiReader2EventStressTest : public MultiReader2Test
{
public:
    struct Gen
    {
        std::int64_t commonMs = 0;  // the common time of the next sample
        int factor = 1;             // ticks per millisecond: 1 (1 kHz tick) or 10 (10 kHz tick, delta 10)
        std::int64_t originMs = 0;  // whole seconds after the base origin
        bool unitVolt = false;
        bool int32 = false;
        bool valueBroken = false;  // ComplexFloat32: not readable as a number
        bool rateBroken = false;   // 500 Hz: does not match the others
        bool connected = true;
    };

    static std::string originFor(std::int64_t originMs)
    {
        const auto total = multi_reader2_stress_test::BASE_SECONDS + originMs / 1000;
        return fmt::format("2022-09-27T{:02}:{:02}:{:02}+00:00", total / 3600, (total / 60) % 60, total % 60);
    }

    static std::int64_t originMsOf(const DataDescriptorPtr& domain)
    {
        const std::string origin = domain.getOrigin();
        const int h = std::stoi(origin.substr(11, 2));
        const int m = std::stoi(origin.substr(14, 2));
        const int s = std::stoi(origin.substr(17, 2));
        return (h * 3600 + m * 60 + s - multi_reader2_stress_test::BASE_SECONDS) * 1000;
    }

    DataDescriptorPtr domainFor(const Gen& g) const
    {
        const Int delta = g.rateBroken ? 2 : g.factor;
        return createDomainDescriptor(originFor(g.originMs), Ratio(1, 1000 * g.factor), LinearDataRule(delta, 0));
    }

    static DataDescriptorPtr valueFor(const Gen& g)
    {
        auto builder = DataDescriptorBuilder().setSampleType(g.valueBroken ? SampleType::ComplexFloat32
                                                             : g.int32     ? SampleType::Int32
                                                                           : SampleType::Float64);
        if (g.unitVolt)
            builder.setUnit(Unit("V"));
        return builder.build();
    }

    // Sends `count` samples of input `i` at its current common time; the value of a sample is its common time
    void sendData(SizeT i, Int count)
    {
        Gen& g = gens[i];
        const auto& read = readSignals[i];
        const std::int64_t ownTick = (g.commonMs - g.originMs) * g.factor;
        auto domainPacket = DataPacket(read.domainDescriptor(), static_cast<SizeT>(count), ownTick);
        auto packet = DataPacketWithDomain(domainPacket, read.signal.getDescriptor(), static_cast<SizeT>(count));
        if (g.int32)
        {
            auto data = static_cast<int32_t*>(packet.getRawData());
            for (Int k = 0; k < count; k++)
                data[k] = static_cast<int32_t>(g.commonMs + k);
        }
        else
        {
            auto data = static_cast<double*>(packet.getRawData());
            for (Int k = 0; k < count; k++)
                data[k] = static_cast<double>(g.commonMs + k);
        }
        g.commonMs += count;
        read.signal.sendPacket(packet);
    }

    void applyDomain(SizeT i)
    {
        readSignals[i].signal.getDomainSignal().asPtr<ISignalConfig>().setDescriptor(domainFor(gens[i]));
    }

    void applyValue(SizeT i)
    {
        readSignals[i].signal.setDescriptor(valueFor(gens[i]));
    }

    // One random operation on one random input
    void step(std::mt19937& rng, const Scenario& sc)
    {
        const SizeT i = rng() % static_cast<unsigned>(sc.inputs);
        Gen& g = gens[i];
        const unsigned pick = rng() % 100;
        const bool silent = g.valueBroken || g.rateBroken || !g.connected;
        if (pick < 45)
        {
            const Int count = 1 + static_cast<Int>(rng() % 40);
            if (silent)
                g.commonMs += count;  // what the broken input would have produced is lost
            else
                sendData(i, count);
        }
        else if (pick < 53)
            g.commonMs += 1 + rng() % 30;  // a gap
        else if (pick < 61)
        {
            g.unitVolt = !g.unitVolt;
            applyValue(i);
        }
        else if (pick < 67)
        {
            g.int32 = !g.int32;
            applyValue(i);
        }
        else if (pick < 71)
        {
            g.valueBroken = !g.valueBroken;
            applyValue(i);
        }
        else if (pick < 77)
        {
            std::int64_t next = g.originMs;
            while (next == g.originMs)
                next = 1000 * (rng() % 4);
            g.originMs = next;
            applyDomain(i);
        }
        else if (pick < 83)
        {
            g.factor = g.factor == 1 ? 10 : 1;
            applyDomain(i);
        }
        else if (pick < 87)
        {
            g.rateBroken = !g.rateBroken;
            applyDomain(i);
        }
        else if (pick < 91)
            readSignals[i].signal.sendPacket(ImplicitDomainGapDetectedEventPacket(7));
        else if (pick < 94)
            readSignals[i].signal.sendPacket(EventPacket("SomethingElse", Dict<IString, IBaseObject>()));
        else if (pick < 97)
        {
            if (!silent)
            {
                auto domainPacket = DataPacket(readSignals[i].domainDescriptor(), 0, (g.commonMs - g.originMs) * g.factor);
                readSignals[i].signal.sendPacket(DataPacketWithDomain(domainPacket, readSignals[i].signal.getDescriptor(), 0));
            }
        }
        else if (sc.overPorts)
        {
            if (g.connected)
                ports[i].disconnect();
            else
                ports[i].connect(readSignals[i].signal);
            g.connected = !g.connected;
        }
    }

    // What one read looked like, for the determinism comparison
    struct Snapshot
    {
        bool valid = false;
        std::vector<MultiReader2InputError> errors;
        bool hadData = false;
        std::int64_t lastTick = 0;
        std::int64_t delta = 1;
        bool first = true;
        DataDescriptorPtr domain;  // the main domain the next read's data is in
    };

    // Reads up to `count` samples with the domain and checks every invariant; appends a line to the trace
    void checkedRead(const MultiReader2Ptr& reader, SizeT count, const Scenario& sc, Snapshot& last, std::ostringstream& trace)
    {
        const SizeT n = static_cast<SizeT>(sc.inputs);
        std::vector<std::int64_t> ticks(count, -1);
        std::vector<std::vector<double>> values(n, std::vector<double>(count, std::nan("")));
        std::vector<void*> buffers(n + 1);
        buffers[0] = ticks.data();
        for (SizeT i = 0; i < n; i++)
            buffers[i + 1] = values[i].data();
        SizeT got = count;
        const auto status = reader.readWithDomain(buffers.data(), &got);
        ASSERT_TRUE(status.assigned());
        ASSERT_LE(got, count);

        const bool valid = status.getValid();
        const bool domainChanged = status.getDomainDescriptorChanged();
        const bool resynchronized = status.getResynchronized();
        const auto inputs = status.getInputs();
        ASSERT_EQ(inputs.getCount(), n);

        bool anyDescriptorChanged = false;
        bool diffs = last.first || valid != last.valid;
        for (SizeT i = 0; i < n; i++)
        {
            const MultiReader2InputStatusPtr in = inputs[i];
            ASSERT_EQ(in.getInput().getGlobalId(), (sc.overPorts ? ports[i].getGlobalId() : readSignals[i].signal.getGlobalId()));
            const bool healthy = valid && in.getError() == MultiReader2InputError::None;
            ASSERT_EQ(in.getDescriptor().assigned(), healthy) << "input " << i;
            if (in.getDescriptorChanged())
            {
                anyDescriptorChanged = true;
                ASSERT_TRUE(healthy) << "a descriptor edge on an unhealthy input " << i;
            }
            if (!last.first && in.getError() != last.errors[i])
                diffs = true;
        }
        const bool edges = domainChanged || resynchronized || anyDescriptorChanged;
        if (!valid)
        {
            ASSERT_EQ(got, 0u);
            ASSERT_FALSE(status.getDomainDescriptor().assigned());
            ASSERT_FALSE(edges);
        }
        if (!status.getHasChanges())
            ASSERT_FALSE(edges || diffs) << "a quiet status with an edge or a level change";
        else if (!last.first)
            ASSERT_TRUE(edges || diffs) << "hasChanges without anything changed";

        std::int64_t delta = 1;
        if (valid)
        {
            // The samples were copied before this read's boundaries took effect: a domain change reported now
            // applies from the next data on, the data of this read is in the domain reported before
            const auto domain = domainChanged && last.domain.assigned() ? last.domain : status.getDomainDescriptor();
            ASSERT_TRUE(domain.assigned());
            const auto resolution = domain.getTickResolution();
            const std::int64_t den = resolution.getDenominator();
            delta = domain.getRule().getParameters()["delta"];
            const std::int64_t originMs = originMsOf(domain);
            for (SizeT k = 0; k < got; k++)
            {
                ASSERT_EQ(ticks[k], ticks[0] + static_cast<std::int64_t>(k) * delta) << "ticks are not a lattice";
                ASSERT_EQ((ticks[k] * 1000) % den, 0) << "a tick off the millisecond";
                const double expected = static_cast<double>(ticks[k] * 1000 / den + originMs);
                for (SizeT i = 0; i < n; i++)
                {
                    const MultiReader2InputStatusPtr in = inputs[i];
                    const bool written = !std::isnan(values[i][k]);
                    const bool healthyNow = in.getError() == MultiReader2InputError::None;
                    const bool healthyBefore = !last.first && last.valid && last.errors[i] == MultiReader2InputError::None;
                    // The samples were copied before the boundaries of this read took effect: the previous status
                    // says who contributed; an error reported now rides behind the data
                    if (written)
                    {
                        ASSERT_EQ(values[i][k], expected) << "input " << i << " sample " << k << " is misaligned";
                        ASSERT_TRUE(last.first || healthyBefore) << "input " << i << " was written although it did not contribute";
                    }
                    else
                        ASSERT_TRUE(!healthyBefore || !healthyNow || resynchronized || domainChanged)
                            << "input " << i << " contributed but was not written";
                }
            }
            if (got > 0 && last.hadData && !resynchronized && !domainChanged)
                ASSERT_EQ(ticks[0], last.lastTick + last.delta) << "the stream jumped without a resynchronization";
        }

        trace << got << ' ' << valid << ' ' << static_cast<bool>(status.getHasChanges()) << ' ' << domainChanged << ' ' << resynchronized
              << ' ';
        for (SizeT i = 0; i < n; i++)
        {
            const MultiReader2InputStatusPtr in = inputs[i];
            trace << static_cast<int>(in.getError()) << static_cast<int>(static_cast<bool>(in.getDescriptorChanged())) << ',';
        }
        if (got > 0)
            trace << " @" << ticks[0];
        trace << '\n';

        last.first = false;
        last.valid = valid;
        last.domain = valid ? status.getDomainDescriptor() : nullptr;
        last.errors.assign(n, MultiReader2InputError::None);
        for (SizeT i = 0; i < n; i++)
        {
            const MultiReader2InputStatusPtr in = inputs[i];
            last.errors[i] = in.getError();
        }
        // A flagged jump materializes in a later read, so continuity is checked again only from the next data on
        if (resynchronized || domainChanged || !valid)
            last.hadData = false;
        else if (got > 0)
        {
            last.hadData = true;
            last.lastTick = ticks[got - 1];
            last.delta = delta;
        }
    }

    // Builds the inputs and the reader of a scenario; the fixture is reused, so the previous run is torn down first
    MultiReader2Ptr setUpScenario(const Scenario& sc)
    {
        readSignals.clear();
        ports.clear();
        gens.assign(static_cast<SizeT>(sc.inputs), Gen{});
        for (auto& g : gens)
            g.commonMs = 10000;  // origins shift by whole seconds, so ticks stay positive
        readSignals.reserve(static_cast<SizeT>(sc.inputs));
        for (int i = 0; i < sc.inputs; i++)
            addSignal(0, 10, createDomainSignal(originFor(0)));
        auto p = params(sc.overPorts ? portsList() : signalsToList());
        p.setErrorPolicy(sc.policy);
        auto reader = createReader(p);
        manager(reader).setSyncLimits(std::chrono::seconds(60), std::chrono::seconds(60));
        if (sc.overPorts)
            for (SizeT i = 0; i < ports.size(); i++)
                ports[i].connect(readSignals[i].signal);
        scheduler.waitAll();
        return reader;
    }

    // Repairs every input and restarts all of them on one common time; the reader must come back whole
    void repairAndVerify(const MultiReader2Ptr& reader, const Scenario& sc, Snapshot& last, std::ostringstream& trace)
    {
        std::int64_t restart = 0;
        for (SizeT i = 0; i < gens.size(); i++)
        {
            Gen& g = gens[i];
            if (g.valueBroken)
            {
                g.valueBroken = false;
                applyValue(i);
            }
            if (g.rateBroken)
            {
                g.rateBroken = false;
                applyDomain(i);
            }
            if (!g.connected)
            {
                ports[i].connect(readSignals[i].signal);
                g.connected = true;
            }
            restart = std::max(restart, g.commonMs);
        }
        scheduler.waitAll();
        restart += 100;
        for (auto& g : gens)
            g.commonMs = restart;

        // A recovery restarts the reader and drops what was queued behind the boundary that brought it back, so the
        // stream has to keep flowing, as a live one does: every attempt sends a round and reads
        bool whole = false;
        for (int attempt = 0; attempt < 40 && !whole; attempt++)
        {
            for (SizeT i = 0; i < gens.size(); i++)
                sendData(i, 50);
            scheduler.waitAll();
            checkedRead(reader, 32, sc, last, trace);
            if (HasFatalFailure())
                return;
            whole = last.valid && reader.getAvailableCount() > 0 &&
                    std::all_of(
                        last.errors.begin(), last.errors.end(), [](MultiReader2InputError e) { return e == MultiReader2InputError::None; });
        }
        ASSERT_TRUE(whole) << "the reader did not come back whole after the repair:\n" << trace.str();
        ASSERT_EQ(reader.getMainInput(), sc.overPorts ? ports[0].getGlobalId() : readSignals[0].signal.getGlobalId());
    }

    // The deterministic run: every operation is drained by the scheduler before the next, reads are interleaved
    std::string runScenario(const Scenario& sc)
    {
        auto reader = setUpScenario(sc);
        std::mt19937 rng(sc.seed);
        Snapshot last;
        std::ostringstream trace;
        checkedRead(reader, 16, sc, last, trace);
        for (int round = 0; round < sc.rounds && !HasFatalFailure(); round++)
        {
            step(rng, sc);
            scheduler.waitAll();
            if (rng() % 2 == 0)
                checkedRead(reader, 1 + rng() % 64, sc, last, trace);
        }
        if (!HasFatalFailure())
            repairAndVerify(reader, sc, last, trace);
        return trace.str();
    }

    std::vector<Gen> gens;
};

class MultiReader2EventScenarioTest : public MultiReader2EventStressTest, public WithParamInterface<Scenario>
{
};

TEST_P(MultiReader2EventScenarioTest, SurvivesAndStaysConsistent)
{
    runScenario(GetParam());
}

TEST_P(MultiReader2EventScenarioTest, IsDeterministic)
{
    const auto first = runScenario(GetParam());
    if (HasFatalFailure())
        return;
    const auto second = runScenario(GetParam());
    ASSERT_EQ(first, second);
}

INSTANTIATE_TEST_SUITE_P(Scenarios,
                         MultiReader2EventScenarioTest,
                         Values(Scenario{"Invalidate12Signals", 1, MultiReader2ErrorPolicy::Invalidate, false, 12, 400},
                                Scenario{"Exclude12Signals", 2, MultiReader2ErrorPolicy::Exclude, false, 12, 400},
                                Scenario{"Invalidate12Ports", 3, MultiReader2ErrorPolicy::Invalidate, true, 12, 400},
                                Scenario{"Exclude12Ports", 4, MultiReader2ErrorPolicy::Exclude, true, 12, 400},
                                Scenario{"Exclude16PortsLong", 5, MultiReader2ErrorPolicy::Exclude, true, 16, 1200},
                                Scenario{"Invalidate10SignalsLong", 6, MultiReader2ErrorPolicy::Invalidate, false, 10, 1200},
                                Scenario{"Exclude24Signals", 7, MultiReader2ErrorPolicy::Exclude, false, 24, 600},
                                Scenario{"Invalidate3Ports", 8, MultiReader2ErrorPolicy::Invalidate, true, 3, 800}),
                         [](const TestParamInfo<Scenario>& info) { return std::string(info.param.name); });

// The producer runs free on its own thread while the reader is drained and checked
TEST_F(MultiReader2EventStressTest, ConcurrentProducerUnderExclude)
{
    const Scenario sc{"Concurrent", 11, MultiReader2ErrorPolicy::Exclude, true, 12, 3000};
    auto reader = setUpScenario(sc);
    std::atomic<bool> done{false};
    std::thread producer(
        [&]
        {
            std::mt19937 rng(sc.seed);
            for (int round = 0; round < sc.rounds; round++)
                step(rng, sc);
            done = true;
        });

    Snapshot last;
    std::ostringstream trace;
    std::mt19937 rng(99);
    while (!done && !HasFatalFailure())
    {
        checkedRead(reader, 1 + rng() % 64, sc, last, trace);
        std::this_thread::yield();
    }
    producer.join();
    scheduler.waitAll();
    if (!HasFatalFailure())
        repairAndVerify(reader, sc, last, trace);
}

TEST_F(MultiReader2EventStressTest, ConcurrentProducerUnderInvalidate)
{
    const Scenario fixed{"Concurrent", 12, MultiReader2ErrorPolicy::Invalidate, false, 12, 3000};
    auto reader = setUpScenario(fixed);
    std::atomic<bool> done{false};
    std::thread producer(
        [&]
        {
            std::mt19937 rng(fixed.seed);
            for (int round = 0; round < fixed.rounds; round++)
                step(rng, fixed);
            done = true;
        });

    Snapshot last;
    std::ostringstream trace;
    std::mt19937 rng(98);
    while (!done && !HasFatalFailure())
    {
        checkedRead(reader, 1 + rng() % 64, fixed, last, trace);
        std::this_thread::yield();
    }
    producer.join();
    scheduler.waitAll();
    if (!HasFatalFailure())
        repairAndVerify(reader, fixed, last, trace);
}

// ---------------------------------------------------------------- scripted interleavings

class MultiReader2InterleavingTest : public MultiReader2EventStressTest
{
public:
    static constexpr int N = 12;

    MultiReader2Ptr twelve(MultiReader2ErrorPolicy policy = MultiReader2ErrorPolicy::Invalidate, bool overPorts = false)
    {
        const Scenario sc{"Scripted", 0, policy, overPorts, N, 0};
        auto reader = setUpScenario(sc);
        probe(reader);
        return reader;
    }

    void sendAll(Int count)
    {
        for (SizeT i = 0; i < static_cast<SizeT>(N); i++)
            sendData(i, count);
        scheduler.waitAll();
    }

    std::vector<std::vector<double>> readAll(const MultiReader2Ptr& reader, SizeT& count, MultiReader2StatusPtr& statusOut)
    {
        std::vector<std::vector<double>> values(N, std::vector<double>(count, -1.0));
        std::vector<void*> buffers(N);
        for (SizeT i = 0; i < static_cast<SizeT>(N); i++)
            buffers[i] = values[i].data();
        SizeT offset = 0;
        statusOut = reader.read(buffers.data(), &count, offset);
        return values;
    }

    std::vector<std::vector<double>> readAll(const MultiReader2Ptr& reader, SizeT& count)
    {
        MultiReader2StatusPtr status;
        return readAll(reader, count, status);
    }

    // Drains the reader into per-input value lists; returns the statuses seen
    std::vector<MultiReader2StatusPtr> drain(const MultiReader2Ptr& reader, std::vector<std::vector<double>>& out, int maxReads = 1000)
    {
        std::vector<MultiReader2StatusPtr> statuses;
        out.assign(N, {});
        int quiet = 0;
        for (int r = 0; r < maxReads && quiet < 3; r++)
        {
            SizeT count = 64;
            MultiReader2StatusPtr status;
            const auto values = readAll(reader, count, status);
            statuses.push_back(status);
            quiet = count == 0 && !status.getHasChanges() ? quiet + 1 : 0;
            for (SizeT i = 0; i < static_cast<SizeT>(N); i++)
                out[i].insert(out[i].end(), values[i].begin(), values[i].begin() + count);
        }
        return statuses;
    }
};

TEST_F(MultiReader2InterleavingTest, TwoValueChangesWithoutDataBetweenReportOnce)
{
    auto reader = twelve();
    sendAll(10);
    gens[3].unitVolt = true;
    applyValue(3);
    gens[3].int32 = true;
    applyValue(3);
    sendAll(10);

    SizeT count = 64;
    MultiReader2StatusPtr status;
    readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(input(status, 3).getDescriptorChanged());
    ASSERT_EQ(input(status, 3).getDescriptor().getSampleType(), SampleType::Int32);
    ASSERT_EQ(input(status, 3).getDescriptor().getUnit().getSymbol(), "V");
    count = 64;
    const auto values = readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getHasChanges());
    ASSERT_EQ(values[3][0], 10010.0);
}

TEST_F(MultiReader2InterleavingTest, ChangeAndChangeBackBeforeAReadLandsOnTheOriginal)
{
    auto reader = twelve();
    sendAll(10);
    const auto original = readSignals[5].signal.getDescriptor();
    gens[5].unitVolt = true;
    applyValue(5);
    gens[5].unitVolt = false;
    applyValue(5);
    sendAll(10);

    SizeT count = 64;
    MultiReader2StatusPtr status;
    readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 5).getDescriptor(), original);
    count = 64;
    readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
}

TEST_F(MultiReader2InterleavingTest, DescriptorChangeThenGapThenDataOnOneInput)
{
    auto reader = twelve();
    sendAll(10);
    gens[7].unitVolt = true;
    applyValue(7);
    gens[7].commonMs += 5;
    sendAll(10);  // input 7 now starts at 15, the others at 10

    std::vector<std::vector<double>> out;
    const auto statuses = drain(reader, out);
    // Ten before the boundary, then the five common samples from 15 on
    for (int i = 0; i < N; i++)
    {
        ASSERT_EQ(out[i].size(), 15u) << i;
        ASSERT_EQ(out[i][9], 10009.0) << i;
        ASSERT_EQ(out[i][10], 10015.0) << i;
        ASSERT_EQ(out[i][14], 10019.0) << i;
    }
    bool sawChange = false;
    bool sawResync = false;
    for (const auto& s : statuses)
    {
        sawChange = sawChange || input(s, 7).getDescriptorChanged();
        sawResync = sawResync || s.getResynchronized();
    }
    ASSERT_TRUE(sawChange);
    ASSERT_TRUE(sawResync);
}

TEST_F(MultiReader2InterleavingTest, BoundariesOnDifferentInputsInOneRead)
{
    auto reader = twelve();
    sendAll(10);
    gens[1].unitVolt = true;
    applyValue(1);
    readSignals[4].signal.sendPacket(ImplicitDomainGapDetectedEventPacket(3));
    gens[8].factor = 10;
    applyDomain(8);
    sendAll(10);

    SizeT count = 64;
    MultiReader2StatusPtr status;
    readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    ASSERT_TRUE(input(status, 1).getDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_FALSE(status.getDomainDescriptorChanged());

    std::vector<std::vector<double>> out;
    drain(reader, out);
    for (int i = 0; i < N; i++)
    {
        ASSERT_EQ(out[i].size(), 10u) << i;
        ASSERT_EQ(out[i][0], 10010.0) << i;
    }
}

TEST_F(MultiReader2InterleavingTest, EveryInputChangesItsDomainAtOnce)
{
    auto reader = twelve();
    sendAll(10);
    for (SizeT i = 0; i < static_cast<SizeT>(N); i++)
    {
        gens[i].originMs = 2000;
        applyDomain(i);
    }
    sendAll(10);

    SizeT count = 64;
    MultiReader2StatusPtr status;
    readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getDomainDescriptorChanged());
    ASSERT_TRUE(status.getResynchronized());
    ASSERT_EQ(originMsOf(status.getDomainDescriptor()), 2000);

    // The main's change reconfigured and dropped what was queued behind the boundaries; new data aligns
    sendAll(10);
    std::vector<std::vector<double>> out;
    drain(reader, out);
    for (int i = 0; i < N; i++)
    {
        ASSERT_FALSE(out[i].empty()) << i;
        ASSERT_EQ(out[i].back(), 10029.0) << i;
    }
}

TEST_F(MultiReader2InterleavingTest, ValueChangesAtDifferentPositionsLoseNoSample)
{
    auto reader = twelve();
    for (int round = 0; round < 20; round++)
    {
        for (SizeT i = 0; i < static_cast<SizeT>(N); i++)
        {
            if (round % (static_cast<int>(i) + 2) == 0)
            {
                gens[i].unitVolt = !gens[i].unitVolt;
                applyValue(i);
            }
            sendData(i, 7);
        }
        scheduler.waitAll();
    }
    std::vector<std::vector<double>> out;
    drain(reader, out);
    for (int i = 0; i < N; i++)
    {
        ASSERT_EQ(out[i].size(), 140u) << i;
        for (SizeT k = 0; k < 140; k++)
            ASSERT_EQ(out[i][k], static_cast<double>(10000 + k)) << i << " " << k;
    }
}

TEST_F(MultiReader2InterleavingTest, GapsAtDifferentPositionsResynchronizeToTheLatest)
{
    auto reader = twelve();
    for (int round = 0; round < 12; round++)
    {
        for (SizeT i = 0; i < static_cast<SizeT>(N); i++)
        {
            if (static_cast<int>(i) == round)
                gens[i].commonMs += 3;
            sendData(i, 10);
        }
        scheduler.waitAll();
    }
    std::vector<std::vector<double>> out;
    drain(reader, out);
    // Every input ends at the same common time; what came out is identical on every input and strictly increasing
    for (int i = 1; i < N; i++)
        ASSERT_EQ(out[i], out[0]) << i;
    for (SizeT k = 1; k < out[0].size(); k++)
        ASSERT_GT(out[0][k], out[0][k - 1]);
    ASSERT_EQ(out[0].back(), static_cast<double>(gens[0].commonMs - 1));
}

TEST_F(MultiReader2InterleavingTest, DisconnectReconnectDisconnectInOneReadEndsDisconnected)
{
    auto reader = twelve(MultiReader2ErrorPolicy::Exclude, true);
    sendAll(10);
    ports[2].disconnect();
    ports[2].connect(readSignals[2].signal);
    ports[2].disconnect();
    scheduler.waitAll();
    SizeT count = 64;
    MultiReader2StatusPtr status;
    readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_TRUE(status.getValid());
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::Disconnected);
    ports[2].connect(readSignals[2].signal);
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_EQ(input(status, 2).getError(), MultiReader2InputError::None);
}

TEST_F(MultiReader2InterleavingTest, EventStormWithoutDataIsOneChange)
{
    auto reader = twelve();
    sendAll(10);
    SizeT count = 64;
    readAll(reader, count);
    for (int k = 0; k < 100; k++)
        for (SizeT i = 0; i < static_cast<SizeT>(N); i++)
        {
            gens[i].unitVolt = !gens[i].unitVolt;
            applyValue(i);
        }
    scheduler.waitAll();
    auto status = probe(reader);
    ASSERT_TRUE(status.getHasChanges());
    ASSERT_TRUE(status.getValid());
    for (int i = 0; i < N; i++)
        ASSERT_FALSE(input(status, i).getDescriptor().getUnit().assigned()) << i;  // toggled an even number of times
    status = probe(reader);
    ASSERT_FALSE(status.getHasChanges());
    sendAll(10);
    count = 64;
    const auto values = readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[11][0], 10010.0);
}

TEST_F(MultiReader2InterleavingTest, BrokenAndRepairedInputsInTurnUnderExclude)
{
    auto reader = twelve(MultiReader2ErrorPolicy::Exclude);
    sendAll(10);
    for (SizeT i = 0; i < static_cast<SizeT>(N); i++)
    {
        gens[i].valueBroken = true;
        applyValue(i);
        scheduler.waitAll();
        std::vector<std::vector<double>> out;
        const auto statuses = drain(reader, out);
        ASSERT_TRUE(statuses.back().getValid()) << i;
        ASSERT_EQ(input(statuses.back(), i).getError(), MultiReader2InputError::ValueDescriptorInvalid) << i;
        gens[i].valueBroken = false;
        applyValue(i);
        scheduler.waitAll();
        auto status = probe(reader);
        ASSERT_EQ(input(status, i).getError(), MultiReader2InputError::None) << i;
        ASSERT_TRUE(status.getValid()) << i;
    }
    sendAll(10);
    std::vector<std::vector<double>> out;
    drain(reader, out);
    for (int i = 0; i < N; i++)
        ASSERT_EQ(out[i].size(), 10u) << i;
}

TEST_F(MultiReader2InterleavingTest, HalfTheInputsBreakAtOnceUnderInvalidateAndAllRecover)
{
    auto reader = twelve();
    sendAll(10);
    // The odd inputs; the main keeps its rate, so the mismatch is theirs
    for (SizeT i = 1; i < static_cast<SizeT>(N); i += 2)
    {
        gens[i].rateBroken = true;
        applyDomain(i);
    }
    sendAll(10);
    SizeT count = 64;
    MultiReader2StatusPtr status;
    readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_FALSE(status.getValid());
    for (int i = 0; i < N; i++)
        ASSERT_EQ(input(status, i).getError(), i % 2 ? MultiReader2InputError::DomainDescriptorInvalid : MultiReader2InputError::None) << i;

    for (SizeT i = 1; i < static_cast<SizeT>(N); i += 2)
    {
        gens[i].rateBroken = false;
        applyDomain(i);
    }
    scheduler.waitAll();
    status = probe(reader);
    ASSERT_TRUE(status.getValid());
    for (int i = 0; i < N; i++)
        ASSERT_EQ(input(status, i).getError(), MultiReader2InputError::None) << i;
    sendAll(10);
    count = 64;
    const auto values = readAll(reader, count, status);
    ASSERT_EQ(count, 10u);
    ASSERT_EQ(values[0][0], 10020.0);
}
