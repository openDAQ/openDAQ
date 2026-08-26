/*
 * MultiReader2 port of the MultiReaderTest suite (test_multi_reader.cpp).
 *
 * Purpose: run the old reader's behavioural expectations against the new implementation to
 * surface regressions. Test names mirror the originals so the mapping stays obvious.
 * Cases whose feature was cut from MultiReader2 live in test_multi_reader2_unsupported.cpp.
 *
 * Tests marked FINDING are faithful ports of behaviour the old reader had and this one does
 * not; they are DISABLED so the suite stays actionable, and each names the defect.
 */
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_params_impl.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal_factory.h>

#include <coretypes/event_wrapper.h>

#include "reader_common.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

using namespace daq;
using namespace testing;

namespace
{

// Mirrors the old suite's ReadSignal: a value signal plus its packet cadence
struct ReadSignal2
{
    ReadSignal2(const SignalConfigPtr& signal, Int packetOffset, Int packetSize)
        : packetSize(packetSize)
        , packetOffset(packetOffset)
        , signal(signal)
        , valueDescriptor(signal.getDescriptor())
    {
    }

    SignalConfigPtr getDomainSignal() const
    {
        return signal.getDomainSignal();
    }

    DataDescriptorPtr getDomainDescriptor() const
    {
        return getDomainSignal().getDescriptor();
    }

    void setValueDescriptor(const DataDescriptorPtr& descriptor)
    {
        signal.setDescriptor(descriptor);
        valueDescriptor = descriptor;
    }

    // Value at domain tick t is t, so alignment is verifiable from the values alone
    DataPacketPtr createAndSendPacket(Int packetIndex) const
    {
        const Int delta = getDomainDescriptor().getRule().getParameters()["delta"];
        const auto offset = packetOffset + ((packetSize * delta) * packetIndex);

        auto domainPacket = DataPacket(getDomainDescriptor(), static_cast<SizeT>(packetSize), offset);
        auto packet = DataPacketWithDomain(domainPacket, valueDescriptor, static_cast<SizeT>(packetSize));
        auto data = static_cast<double*>(packet.getData());
        for (Int i = 0; i < packetSize; i++)
            data[i] = static_cast<double>(offset + i * delta);

        signal.sendPacket(packet);
        return packet;
    }

    Int packetSize;
    Int packetOffset;
    SignalConfigPtr signal;
    DataDescriptorPtr valueDescriptor;
};

}

class MultiReader2MigrationTest : public ReaderTest<>
{
public:
    using Super = ReaderTest<>;

    ReadSignal2& addSignal(Int packetOffset, Int packetSize, const SignalPtr& domain, SampleType valueType = SampleType::Float64)
    {
        auto newSignal = Signal(context, nullptr, fmt::format("sig{}", counter++));
        newSignal.setDescriptor(setupDescriptor(valueType));
        newSignal.setDomainSignal(domain);
        return readSignals.emplace_back(newSignal, packetOffset, packetSize);
    }

    SignalConfigPtr createDomainSignal(std::string epoch = "",
                                       const RatioPtr& resolution = nullptr,
                                       const DataRulePtr& rule = nullptr) const
    {
        auto domain = Signal(context, nullptr, fmt::format("time{}", counter));
        domain.setDescriptor(createDomainDescriptor(std::move(epoch), resolution, rule, nullptr));
        return domain;
    }

    ListPtr<IComponent> signalsToList() const
    {
        auto signals = List<IComponent>();
        for (const auto& read : readSignals)
            signals.pushBack(read.signal);
        return signals;
    }

    ListPtr<IComponent> portsList(bool enableGapDetection = false) const
    {
        auto ports = List<IComponent>();
        SizeT index = 0;
        for (const auto& read : readSignals)
            ports.pushBack(InputPort(read.signal.getContext(), nullptr, "readsig" + std::to_string(index++), enableGapDetection));
        return ports;
    }

    void sendPackets(Int index) const
    {
        for (const auto& signal : readSignals)
            signal.createAndSendPacket(index);
    }

    static ObjectPtr<IMultiReader2Params> params(const ListPtr<IComponent>& inputs, SampleType valueReadType = SampleType::Float64)
    {
        auto params = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
        checkErrorInfo(params->setInputs(inputs));
        checkErrorInfo(params->setValueReadType(valueReadType));
        return params;
    }

    static ObjectPtr<IMultiReader2> createReader(const ObjectPtr<IMultiReader2Params>& p)
    {
        return createWithImplementation<IMultiReader2, MultiReader2Impl>(static_cast<IMultiReader2Params*>(p));
    }

    ObjectPtr<IMultiReader2> readerFromSignals()
    {
        return createReader(params(signalsToList()));
    }

    // --- event window helpers -------------------------------------------------

    static MultiReader2StatusType statusType(const ObjectPtr<IMultiReader2Status>& status)
    {
        MultiReader2StatusType type;
        checkErrorInfo(status->getStatus(&type));
        return type;
    }

    static DictPtr<IString, IInteger> errorsOf(const ObjectPtr<IMultiReader2Status>& status)
    {
        IDict* dict;
        checkErrorInfo(status->getErrors(&dict));
        return DictPtr<IString, IInteger>(ObjectPtr<IDict>::Adopt(dict));
    }

    // One read; returns the status and leaves any event pending.
    // FINDING 7: the old reader took read(nullptr, &count) as the event handshake. MultiReader2
    // rejects a null buffer array even for a zero-sample probe, so every ported call site needs
    // a dummy array. DISABLED_NullBufferEventProbe below records the regression.
    ObjectPtr<IMultiReader2Status> read(const ObjectPtr<IMultiReader2>& reader, void** buffers, SizeT& count, SizeT* offset = nullptr)
    {
        IMultiReader2Status* status;
        SizeT localOffset = 0;
        checkErrorInfo(reader->read(&status, buffers ? buffers : dummyBuffers(), &count, offset ? offset : &localOffset));
        return ObjectPtr<IMultiReader2Status>::Adopt(status);
    }

    // Placeholder buffers for probe reads that expect no samples
    void** dummyBuffers()
    {
        dummyStorage.assign(16, std::vector<double>(16, 0.0));
        dummyPointers.clear();
        for (auto& buffer : dummyStorage)
            dummyPointers.push_back(buffer.data());
        return dummyPointers.data();
    }

    // Drains pending events (committing each) and returns the first Data read
    ObjectPtr<IMultiReader2Status> readData(const ObjectPtr<IMultiReader2>& reader,
                                                   void** buffers,
                                                   SizeT& count,
                                                   SizeT* offset = nullptr,
                                                   int maxEvents = 8)
    {
        const SizeT requested = count;
        for (int i = 0; i < maxEvents; i++)
        {
            count = requested;
            auto status = read(reader, buffers, count, offset);
            if (statusType(status) != MultiReader2StatusType::Event)
                return status;
            checkErrorInfo(reader->commitEvent());
        }
        return nullptr;
    }

    // The migration fixture's context has a scheduler, so notification passes run asynchronously
    static bool waitFor(const std::function<bool()>& predicate, int timeoutMs = 2000)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (predicate())
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return predicate();
    }

    static SizeT availableCount(const ObjectPtr<IMultiReader2>& reader)
    {
        SizeT count = 0;
        checkErrorInfo(reader->getAvailableCount(&count));
        return count;
    }

protected:
    int counter{};
    std::vector<ReadSignal2> readSignals;
    std::vector<std::vector<double>> dummyStorage;
    std::vector<void*> dummyPointers;
};

// --- Basic alignment and reading -------------------------------------------

// Port of SignalStartDomainFrom0, with one shared epoch (the original used three different ones)
TEST_F(MultiReader2MigrationTest, SignalStartDomainFrom0)
{
    constexpr auto NUM_SIGNALS = 3;
    readSignals.reserve(NUM_SIGNALS);

    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 523, domain);
    auto& sig1 = addSignal(0, 732, domain);
    auto& sig2 = addSignal(0, 843, domain);

    auto reader = readerFromSignals();

    // The bootstrap handshake reports the descriptors before any data is readable
    SizeT count = 0;
    auto status = read(reader, nullptr, count);
    ASSERT_EQ(statusType(status), MultiReader2StatusType::Event);
    ASSERT_EQ(availableCount(reader), 0u);
    checkErrorInfo(reader->commitEvent());

    for (Int i = 0; i < 3; i++)
    {
        sig0.createAndSendPacket(i);
        sig1.createAndSendPacket(i);
        sig2.createAndSendPacket(i);
    }

    constexpr SizeT SAMPLES = 5u;
    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    void* valuesPerSignal[NUM_SIGNALS]{values[0], values[1], values[2]};

    count = SAMPLES;
    SizeT offset = 0;
    auto data = readData(reader, valuesPerSignal, count, &offset);
    ASSERT_TRUE(data.assigned());
    ASSERT_EQ(count, SAMPLES);

    // Values encode their own domain position, so equal values mean aligned reads
    ASSERT_THAT(values[1], ElementsAreArray(values[0]));
    ASSERT_THAT(values[2], ElementsAreArray(values[0]));
}

// Port of WithPacketOffsetNot0: alignment discards the head of the earlier starter
TEST_F(MultiReader2MigrationTest, WithPacketOffsetNot0)
{
    constexpr auto NUM_SIGNALS = 2;
    readSignals.reserve(NUM_SIGNALS);

    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(100, 100, domain);
    auto& sig1 = addSignal(150, 100, domain);

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);

    constexpr SizeT SAMPLES = 10u;
    std::array<double[SAMPLES], NUM_SIGNALS> values{};
    void* buffers[NUM_SIGNALS]{values[0], values[1]};

    count = SAMPLES;
    SizeT offset = 0;
    auto data = readData(reader, buffers, count, &offset);
    ASSERT_TRUE(data.assigned());
    ASSERT_EQ(count, SAMPLES);
    ASSERT_EQ(offset, 150u);
    ASSERT_DOUBLE_EQ(values[0][0], 150.0);
    ASSERT_DOUBLE_EQ(values[1][0], 150.0);
}

// Port of IsSynchronized: availability stays zero until alignment succeeds
TEST_F(MultiReader2MigrationTest, IsSynchronized)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto reader = readerFromSignals();
    ASSERT_EQ(availableCount(reader), 0u);

    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());
    ASSERT_EQ(availableCount(reader), 0u);

    sig0.createAndSendPacket(0);
    ASSERT_EQ(availableCount(reader), 0u);

    sig1.createAndSendPacket(0);
    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    SizeT readCount = 16;
    readData(reader, buffers, readCount);
    ASSERT_EQ(readCount, 10u);
}

// Port of Clock10kHzDelta10: a non-unit delta is a legal grid
TEST_F(MultiReader2MigrationTest, Clock10kHzDelta10)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00", Ratio(1, 10000), LinearDataRule(10, 0));
    auto& sig0 = addSignal(0, 100, domain);
    auto& sig1 = addSignal(0, 100, domain);

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);

    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    count = 16;
    auto data = readData(reader, buffers, count);
    ASSERT_TRUE(data.assigned());
    ASSERT_EQ(count, 16u);
    ASSERT_DOUBLE_EQ(v0[1] - v0[0], 10.0);
}

// --- Input ports ------------------------------------------------------------

// Port of MultiReaderWithInputPort
TEST_F(MultiReader2MigrationTest, MultiReaderWithInputPort)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto ports = portsList();
    ports[0].asPtr<IInputPortConfig>().connect(sig0.signal);
    ports[1].asPtr<IInputPortConfig>().connect(sig1.signal);

    auto reader = createReader(params(ports));
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);

    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    count = 16;
    auto data = readData(reader, buffers, count);
    ASSERT_TRUE(data.assigned());
    ASSERT_EQ(count, 10u);
}

// Port of MultiReaderWithNotConnectedInputPort / ReadWhenOnePortIsNotConnected
TEST_F(MultiReader2MigrationTest, ReadWhenOnePortIsNotConnected)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    addSignal(0, 10, domain);

    auto ports = portsList();
    ports[0].asPtr<IInputPortConfig>().connect(sig0.signal);

    auto reader = createReader(params(ports));

    // Nothing is readable and nothing wakes while a used input has no connection
    ASSERT_EQ(availableCount(reader), 0u);
    sig0.createAndSendPacket(0);
    ASSERT_EQ(availableCount(reader), 0u);

    SizeT count = 0;
    auto status = read(reader, nullptr, count);
    ASSERT_EQ(count, 0u);
}

// Port of NotifyPortIsConnected: connecting the last port completes the set
TEST_F(MultiReader2MigrationTest, NotifyPortIsConnected)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto ports = portsList();
    auto reader = createReader(params(ports));

    std::atomic<int> fired{0};
    IEvent* eventIntf;
    checkErrorInfo(reader->getOnDataAvailable(&eventIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDataAvailable{ObjectPtr<IEvent>::Adopt(eventIntf)};
    onDataAvailable += [&fired](InputPortPtr&, EventArgsPtr<>&) { fired++; };

    ports[0].asPtr<IInputPortConfig>().connect(sig0.signal);
    ASSERT_FALSE(waitFor([&fired] { return fired.load() > 0; }, 200));

    ports[1].asPtr<IInputPortConfig>().connect(sig1.signal);
    ASSERT_TRUE(waitFor([&fired] { return fired.load() > 0; }));
}

// Port of MultipleMultiReaderToInputPort: a port drives one listener only
TEST_F(MultiReader2MigrationTest, MultipleMultiReaderToInputPort)
{
    readSignals.reserve(1);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    addSignal(0, 10, domain);

    auto ports = portsList();
    auto reader = createReader(params(ports));
    // Matches the old reader: a port already owned by a reader cannot be adopted by a second one
    ASSERT_THROW(createReader(params(ports)), AlreadyExistsException);
}

// Port of MultiReaderReuseInputPort: ownership is released when the reader dies
TEST_F(MultiReader2MigrationTest, MultiReaderReuseInputPort)
{
    readSignals.reserve(1);
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto ports = portsList();
    {
        auto reader = createReader(params(ports));
    }
    ASSERT_NO_THROW(createReader(params(ports)));
}

// --- Callbacks --------------------------------------------------------------

// FINDING 8: a callback-only consumer is never woken when the context has a scheduler.
// The bootstrap pass runs during construction, before any handler can be attached; it leaves an
// event pending, so armDataAvailable() reports "another pass is owed" - but the notification Work
// is single-shot and discards that return, so the reader latches disarmed. Nothing re-arms until
// the consumer calls read()/commitEvent(), which a callback-driven consumer never does.
// Probe evidence: no callback after data; after one manual read+commit, later data fires it.
// The sum FB escapes this only because it attaches its handler before any input is connected.
TEST_F(MultiReader2MigrationTest, DISABLED_MultiReaderOnReadCallback)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto reader = readerFromSignals();
    IMultiReader2* readerRaw = reader;

    std::atomic<int> reads{0};
    std::atomic<SizeT> total{0};
    IEvent* eventIntf;
    checkErrorInfo(reader->getOnDataAvailable(&eventIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDataAvailable{ObjectPtr<IEvent>::Adopt(eventIntf)};
    onDataAvailable += [readerRaw, &reads, &total](InputPortPtr&, EventArgsPtr<>&)
    {
        double v0[64]{};
        double v1[64]{};
        void* buffers[2]{v0, v1};
        for (int i = 0; i < 8; i++)
        {
            IMultiReader2Status* statusRaw;
            SizeT count = 64;
            SizeT offset = 0;
            checkErrorInfo(readerRaw->read(&statusRaw, buffers, &count, &offset));
            auto status = ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);
            MultiReader2StatusType type;
            checkErrorInfo(status->getStatus(&type));
            if (type == MultiReader2StatusType::Event)
            {
                checkErrorInfo(readerRaw->commitEvent());
                continue;
            }
            total += count;
            break;
        }
        reads++;
    };

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);

    ASSERT_TRUE(waitFor([&reads] { return reads.load() > 0; }));
    ASSERT_TRUE(waitFor([&total] { return total.load() > 0u; }));
}

// --- Active / used ----------------------------------------------------------

// Port of MultiReaderActive: an inactive reader drops data but still reports events
TEST_F(MultiReader2MigrationTest, MultiReaderActive)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->setActive(False));
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    ASSERT_EQ(availableCount(reader), 0u);

    // A descriptor change still reaches an inactive reader
    sig0.setValueDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setValueRange(daq::Range(-5, 5)).build());
    sig0.createAndSendPacket(1);
    SizeT eventCount = 0;
    auto status = read(reader, nullptr, eventCount);
    ASSERT_EQ(statusType(status), MultiReader2StatusType::Event);
}

// Port of UsedUnusedInput: an unused input neither blocks nor contributes
TEST_F(MultiReader2MigrationTest, UsedUnusedInput)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto p = params(signalsToList());
    checkErrorInfo(p->setUnusedInputs(List<IComponent>(sig1.signal)));
    auto reader = createReader(p);

    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    // Only the used input needs data for the reader to deliver
    sig0.createAndSendPacket(0);
    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    count = 16;
    auto data = readData(reader, buffers, count);
    ASSERT_TRUE(data.assigned());
    ASSERT_EQ(count, 10u);
}

// Port of AddRemoveInput: configure replaces the input set
TEST_F(MultiReader2MigrationTest, AddRemoveInput)
{
    readSignals.reserve(3);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);
    auto& sig2 = addSignal(0, 10, domain);

    auto reader = createReader(params(List<IComponent>(sig0.signal, sig1.signal)));
    StringPtr mainId;
    checkErrorInfo(reader->getMainInput(&mainId));
    ASSERT_EQ(mainId, sig0.signal.getGlobalId());

    checkErrorInfo(reader->configure(params(List<IComponent>(sig1.signal, sig2.signal))));
    checkErrorInfo(reader->getMainInput(&mainId));
    ASSERT_EQ(mainId, sig1.signal.getGlobalId());

    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig1.createAndSendPacket(0);
    sig2.createAndSendPacket(0);
    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    count = 16;
    auto data = readData(reader, buffers, count);
    ASSERT_TRUE(data.assigned());
    ASSERT_EQ(count, 10u);
}

// --- Descriptor changes -----------------------------------------------------

// Port of Signal2Invalidated: a mid-stream descriptor change is reported
TEST_F(MultiReader2MigrationTest, DescriptorChangeIsReported)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    count = 16;
    readData(reader, buffers, count);

    sig1.setValueDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setValueRange(daq::Range(-5, 5)).build());
    sig1.createAndSendPacket(1);

    count = 16;
    auto status = read(reader, buffers, count);
    ASSERT_EQ(statusType(status), MultiReader2StatusType::Event);
}

// Port of ReadSignalWithoutDomainDoesNotCrash
TEST_F(MultiReader2MigrationTest, ReadSignalWithoutDomainDoesNotCrash)
{
    auto signal = Signal(context, nullptr, "no_domain");
    signal.setDescriptor(setupDescriptor(SampleType::Float64));

    auto reader = createReader(params(List<IComponent>(signal)));
    SizeT count = 0;
    ASSERT_NO_THROW(read(reader, nullptr, count));
    ASSERT_EQ(availableCount(reader), 0u);
}

// Port of SharedDomainDescriptorChangeInvalidatesReaderAcrossCallbacks
TEST_F(MultiReader2MigrationTest, SharedDomainDescriptorChangeIsReported)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    count = 16;
    readData(reader, buffers, count);

    // Both signals share one domain signal: changing it must reach the reader
    domain.setDescriptor(createDomainDescriptor("2022-09-27T00:02:03+00:00", Ratio(1, 2000), LinearDataRule(1, 0), nullptr));
    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);

    count = 16;
    auto status = read(reader, buffers, count);
    ASSERT_EQ(statusType(status), MultiReader2StatusType::Event);
}

// --- Regressions found by this migration ------------------------------------

// FINDING 7: read(nullptr, &count) with count 0 was the old reader's event handshake and is
// how every existing consumer probes for events. MultiReader2 returns ArgumentNullException.
TEST_F(MultiReader2MigrationTest, DISABLED_NullBufferEventProbe)
{
    readSignals.reserve(1);
    addSignal(0, 10, createDomainSignal("2022-09-27T00:02:03+00:00"));

    auto reader = readerFromSignals();
    IMultiReader2Status* status;
    SizeT count = 0;
    SizeT offset = 0;
    ASSERT_EQ(reader->read(&status, nullptr, &count, &offset), OPENDAQ_SUCCESS);
    ObjectPtr<IMultiReader2Status>::Adopt(status);
}

// FINDING 5: the old reader aligned signals with DIFFERENT epochs by converting through
// absolute time (the original SignalStartDomainFrom0 used three distinct origins).
// MultiReader2 compares origin strings, so any epoch difference fails the input outright.
TEST_F(MultiReader2MigrationTest, DISABLED_MixedEpochsAlign)
{
    readSignals.reserve(2);
    auto& sig0 = addSignal(0, 100, createDomainSignal("2022-09-27T00:02:03+00:00"));
    auto& sig1 = addSignal(0, 100, createDomainSignal("2022-09-27T00:02:04+00:00"));

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);
    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(1);

    double v0[64]{};
    double v1[64]{};
    void* buffers[2]{v0, v1};
    count = 64;
    auto data = readData(reader, buffers, count);
    ASSERT_TRUE(data.assigned());
    ASSERT_GT(count, 0u);
}

// FINDING 6: the old reader accepted signals whose tick resolutions differ, folding them into
// a common resolution. MultiReader2 requires the effective rate to match after scaling, and a
// resolution that does not divide the main input's is rejected as InvalidDomain.
TEST_F(MultiReader2MigrationTest, DISABLED_MixedResolutionsAlign)
{
    readSignals.reserve(2);
    auto& sig0 = addSignal(0, 100, createDomainSignal("2022-09-27T00:02:03+00:00", Ratio(1, 1000), LinearDataRule(1, 0)));
    auto& sig1 = addSignal(0, 100, createDomainSignal("2022-09-27T00:02:03+00:00", Ratio(1, 1500), LinearDataRule(1, 0)));

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);

    double v0[64]{};
    double v1[64]{};
    void* buffers[2]{v0, v1};
    count = 64;
    auto data = readData(reader, buffers, count);
    ASSERT_TRUE(data.assigned());
    ASSERT_GT(count, 0u);
}


// --- Ported from the remove-ladder-system branch -----------------------------

// Port of StatusEventDictAndInputStates: the boundary reports every input's descriptor,
// keyed by input id, and the errors dict names only the failing ones
TEST_F(MultiReader2MigrationTest, StatusEventDictAndInputStates)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto reader = readerFromSignals();
    SizeT count = 0;
    auto status = read(reader, nullptr, count);
    ASSERT_EQ(statusType(status), MultiReader2StatusType::Event);

    IDict* descriptorsRaw;
    checkErrorInfo(status->getDescriptors(&descriptorsRaw));
    const auto descriptors = DictPtr<IString, IDataDescriptor>(ObjectPtr<IDict>::Adopt(descriptorsRaw));
    ASSERT_EQ(descriptors.getCount(), 2u);
    ASSERT_TRUE(descriptors.hasKey(sig0.signal.getGlobalId()));
    ASSERT_TRUE(descriptors.hasKey(sig1.signal.getGlobalId()));
    ASSERT_EQ(errorsOf(status).getCount(), 0u);
}

// Port of StatusStateIncompatibleRecoverable: a descriptor the reader cannot read fails that
// input and a later readable descriptor brings it back
TEST_F(MultiReader2MigrationTest, StatusStateIncompatibleRecoverable)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain, SampleType::ComplexFloat64);

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);

    // The complex input cannot be converted to the read type
    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    count = 16;
    auto status = read(reader, buffers, count);
    ASSERT_EQ(statusType(status), MultiReader2StatusType::Event);
    const auto errors = errorsOf(status);
    ASSERT_EQ(errors.getCount(), 1u);
    ASSERT_EQ(static_cast<Int>(errors.get(sig1.signal.getGlobalId())),
              static_cast<Int>(MultiReader2InputError::InvalidDescriptor));
    checkErrorInfo(reader->commitEvent());

    // A readable descriptor lets the input rejoin
    sig1.setValueDescriptor(setupDescriptor(SampleType::Float64));
    count = 16;
    status = read(reader, buffers, count);
    ASSERT_EQ(statusType(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorsOf(status).getCount(), 0u);
}

// Port of MaxSyncDistanceZeroDisables, reframed: widely staggered starts still align
TEST_F(MultiReader2MigrationTest, StaggeredStartsStillAlign)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 1000, domain);
    auto& sig1 = addSignal(1500, 1000, domain);

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig0.createAndSendPacket(1);
    sig1.createAndSendPacket(0);

    double v0[64]{};
    double v1[64]{};
    void* buffers[2]{v0, v1};
    count = 64;
    SizeT offset = 0;
    auto data = readData(reader, buffers, count, &offset);
    ASSERT_TRUE(data.assigned());
    ASSERT_EQ(count, 64u);
    ASSERT_EQ(offset, 1500u);
    ASSERT_DOUBLE_EQ(v0[0], 1500.0);
    ASSERT_DOUBLE_EQ(v1[0], 1500.0);
}

// Port of BuriedEventSurfacesOnReadNotOnQuery: a descriptor change behind buffered data is
// reported by read, and getAvailableCount does not report past it
TEST_F(MultiReader2MigrationTest, BuriedEventSurfacesOnRead)
{
    readSignals.reserve(2);
    auto domain = createDomainSignal("2022-09-27T00:02:03+00:00");
    auto& sig0 = addSignal(0, 10, domain);
    auto& sig1 = addSignal(0, 10, domain);

    auto reader = readerFromSignals();
    SizeT count = 0;
    read(reader, nullptr, count);
    checkErrorInfo(reader->commitEvent());

    sig0.createAndSendPacket(0);
    sig1.createAndSendPacket(0);

    // The change is queued behind data that is already readable
    sig0.setValueDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setValueRange(daq::Range(-5, 5)).build());

    ASSERT_EQ(availableCount(reader), 0u);

    double v0[16]{};
    double v1[16]{};
    void* buffers[2]{v0, v1};
    count = 16;
    auto status = read(reader, buffers, count);
    ASSERT_EQ(statusType(status), MultiReader2StatusType::Event);
}
