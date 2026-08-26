#include <coreobjects/unit_factory.h>
#include <coretypes/ratio_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_params_impl.h>
#include <opendaq/multi_reader_data_manager.h>
#include <opendaq/event_packet_utils.h>
#include <opendaq/multi_reader2_status_impl.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal_factory.h>

#include <coretypes/event_wrapper.h>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

using namespace daq;

static MultiReader2StatusType typeOf(const ObjectPtr<IMultiReader2Status>& status)
{
    MultiReader2StatusType type;
    checkErrorInfo(status->getStatus(&type));
    return type;
}

static DictPtr<IString, IDataDescriptor> descriptorsOf(const ObjectPtr<IMultiReader2Status>& status)
{
    IDict* dict;
    checkErrorInfo(status->getDescriptors(&dict));
    return DictPtr<IString, IDataDescriptor>(ObjectPtr<IDict>::Adopt(dict));
}

static DictPtr<IString, IInteger> errorsOf(const ObjectPtr<IMultiReader2Status>& status)
{
    IDict* dict;
    checkErrorInfo(status->getErrors(&dict));
    return DictPtr<IString, IInteger>(ObjectPtr<IDict>::Adopt(dict));
}

static ObjectPtr<IMultiReader2Status> readStatus(IMultiReader2* reader)
{
    IMultiReader2Status* status;
    void* data[8] = {};
    SizeT count = 0;
    SizeT offset = 0;
    checkErrorInfo(reader->read(&status, data, &count, &offset));
    return ObjectPtr<IMultiReader2Status>::Adopt(status);
}

static ObjectPtr<IMultiReader2Status> readStatus(MultiReaderDataManager& manager)
{
    IMultiReader2Status* status;
    void* data[8] = {};
    SizeT count = 0;
    SizeT offset = 0;
    checkErrorInfo(manager.read(&status, data, &count, &offset));
    return ObjectPtr<IMultiReader2Status>::Adopt(status);
}

class MultiReader2Test : public testing::Test
{
protected:
    void SetUp() override
    {
        context = NullContext();
    }

    SignalConfigPtr createSignal(const std::string& localId)
    {
        auto signal = Signal(context, nullptr, localId);
        signal.setDescriptor(descriptor());
        return signal;
    }

    static DataDescriptorPtr descriptor()
    {
        return DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    }

    SignalConfigPtr createSignalWithDomain(const std::string& localId, Int delta = 1)
    {
        auto domainSignal = Signal(context, nullptr, localId + "_domain");
        domainSignal.setDescriptor(DataDescriptorBuilder()
                                       .setSampleType(SampleType::Int64)
                                       .setUnit(Unit("s", -1, "second", "time"))
                                       .setTickResolution(Ratio(1, 1000))
                                       .setRule(LinearDataRule(delta, 0))
                                       .build());
        auto signal = createSignal(localId);
        signal.setDomainSignal(domainSignal);
        return signal;
    }

    // Values mirror the timestamps so reads are verifiable: sample at tick t carries the value t
    static void sendData(const SignalConfigPtr& signal, Int offset, SizeT samples)
    {
        auto domainPacket = DataPacket(signal.getDomainSignal().getDescriptor(), samples, offset);
        auto packet = DataPacketWithDomain(domainPacket, signal.getDescriptor(), samples);
        const auto values = static_cast<double*>(packet.getData());
        for (SizeT i = 0; i < samples; i++)
            values[i] = static_cast<double>(offset) + static_cast<double>(i);
        signal.sendPacket(packet);
    }

    static ObjectPtr<IMultiReader2Params> createParams(const ListPtr<IComponent>& inputs,
                                                       SampleType valueReadType = SampleType::Float64)
    {
        auto params = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
        checkErrorInfo(params->setInputs(inputs));
        if (valueReadType != SampleType::Invalid)
            checkErrorInfo(params->setValueReadType(valueReadType));
        return params;
    }

    static ObjectPtr<IMultiReader2> createReader(const ObjectPtr<IMultiReader2Params>& params)
    {
        return createWithImplementation<IMultiReader2, MultiReader2Impl>(static_cast<IMultiReader2Params*>(params));
    }

    static StringPtr mainInputOf(const ObjectPtr<IMultiReader2>& reader)
    {
        StringPtr id;
        checkErrorInfo(reader->getMainInput(&id));
        return id;
    }

    ContextPtr context;
};

// --- Params validation ---

TEST_F(MultiReader2Test, ParamsRejectMixedInputs)
{
    auto signal = createSignal("sig");
    auto port = InputPort(context, nullptr, "port");
    auto params = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    ASSERT_EQ(params->setInputs(List<IComponent>(signal, port)), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2Test, ParamsRejectZeroMinReadCount)
{
    auto params = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    ASSERT_EQ(params->setMinReadCount(0), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
}

TEST_F(MultiReader2Test, ValueReadTypeIsMandatory)
{
    auto params = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    SampleType type;
    ASSERT_EQ(params->getValueReadType(&type), OPENDAQ_ERR_NOTASSIGNED);
    daqClearErrorInfo();

    checkErrorInfo(params->setValueReadType(SampleType::Float64));
    ASSERT_EQ(params->getValueReadType(&type), OPENDAQ_SUCCESS);
    ASSERT_EQ(type, SampleType::Float64);
}

// --- Construction ---

TEST_F(MultiReader2Test, CreateFromSignals)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto reader = createReader(createParams(List<IComponent>(sig1, sig2)));
    ASSERT_EQ(mainInputOf(reader), sig1.getGlobalId());
}

TEST_F(MultiReader2Test, ExplicitMainInput)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto params = createParams(List<IComponent>(sig1, sig2));
    checkErrorInfo(params->setMainInput(sig2));
    auto reader = createReader(params);
    ASSERT_EQ(mainInputOf(reader), sig2.getGlobalId());
}

TEST_F(MultiReader2Test, CreateWithoutValueReadTypeThrows)
{
    auto sig = createSignal("sig");
    auto params = createParams(List<IComponent>(sig), SampleType::Invalid);
    ASSERT_THROW(createReader(params), NotAssignedException);
}

TEST_F(MultiReader2Test, CreateWithEmptyInputsThrows)
{
    auto params = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
    checkErrorInfo(params->setValueReadType(SampleType::Float64));
    ASSERT_THROW(createReader(params), InvalidParameterException);
}

TEST_F(MultiReader2Test, CreateWithDuplicateInputsThrows)
{
    auto sig = createSignal("sig");
    auto params = createParams(List<IComponent>(sig, sig));
    ASSERT_THROW(createReader(params), DuplicateItemException);
}

TEST_F(MultiReader2Test, MainInputMustBeInInputList)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto params = createParams(List<IComponent>(sig1));
    checkErrorInfo(params->setMainInput(sig2));
    ASSERT_THROW(createReader(params), InvalidParameterException);
}

TEST_F(MultiReader2Test, UnusedInputMustBeInInputList)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto params = createParams(List<IComponent>(sig1));
    checkErrorInfo(params->setUnusedInputs(List<IComponent>(sig2)));
    ASSERT_THROW(createReader(params), InvalidParameterException);
}

TEST_F(MultiReader2Test, MainInputCannotBeUnused)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto params = createParams(List<IComponent>(sig1, sig2));
    checkErrorInfo(params->setUnusedInputs(List<IComponent>(sig1)));
    ASSERT_THROW(createReader(params), InvalidParameterException);
}

// --- Reconfiguration ---

TEST_F(MultiReader2Test, ReconfigureReplacesInputs)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto sig3 = createSignal("sig3");

    auto reader = createReader(createParams(List<IComponent>(sig1, sig2)));
    auto newParams = createParams(List<IComponent>(sig2, sig3));
    checkErrorInfo(reader->configure(newParams));
    ASSERT_EQ(mainInputOf(reader), sig2.getGlobalId());
}

TEST_F(MultiReader2Test, FailedReconfigureKeepsReader)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");

    auto reader = createReader(createParams(List<IComponent>(sig1)));
    auto badParams = createParams(List<IComponent>(sig2));
    checkErrorInfo(badParams->setMainInput(sig1));
    ASSERT_EQ(reader->configure(badParams), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();
    ASSERT_EQ(mainInputOf(reader), sig1.getGlobalId());
}

// --- Gates without a pending event ---

TEST_F(MultiReader2Test, GatedCallsRequireEventWindow)
{
    auto sig = createSignal("sig");
    auto reader = createReader(createParams(List<IComponent>(sig)));

    ASSERT_EQ(reader->commitEvent(), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();
    ASSERT_EQ(reader->setActive(False), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();
    ASSERT_EQ(reader->setUsed(sig.getGlobalId(), False), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();
}

TEST_F(MultiReader2Test, ReadsAlignedData)
{
    auto sig1 = createSignalWithDomain("sig1");
    auto sig2 = createSignalWithDomain("sig2");
    auto reader = createReader(createParams(List<IComponent>(sig1, sig2)));

    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());

    sendData(sig1, 100, 10);
    sendData(sig2, 104, 10);

    double b1[16] = {};
    double b2[16] = {};
    void* buffers[2] = {b1, b2};
    IMultiReader2Status* statusRaw;
    SizeT count = 16;
    SizeT offset = 0;
    checkErrorInfo(reader->read(&statusRaw, buffers, &count, &offset));
    auto status = ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);

    // Sync lands on the latest start; both buffers begin at the same timestamp
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Data);
    ASSERT_EQ(count, 6u);
    ASSERT_EQ(offset, 104u);
    ASSERT_DOUBLE_EQ(b1[0], 104.0);
    ASSERT_DOUBLE_EQ(b2[0], 104.0);
    ASSERT_DOUBLE_EQ(b1[5], 109.0);
    ASSERT_DOUBLE_EQ(b2[5], 109.0);
}

TEST_F(MultiReader2Test, ReadWithDomainReturnsTimestamps)
{
    auto sig = createSignalWithDomain("sig");
    auto reader = createReader(createParams(List<IComponent>(sig)));

    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());

    sendData(sig, 50, 5);

    int64_t timestamps[8] = {};
    double values[8] = {};
    void* buffers[2] = {timestamps, values};
    IMultiReader2Status* statusRaw;
    SizeT count = 8;
    checkErrorInfo(reader->readWithDomain(&statusRaw, buffers, &count));
    auto status = ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);

    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Data);
    ASSERT_EQ(count, 5u);
    ASSERT_EQ(timestamps[0], 50);
    ASSERT_EQ(timestamps[4], 54);
    ASSERT_DOUBLE_EQ(values[2], 52.0);
}

// --- Event pipeline ---

TEST_F(MultiReader2Test, BootstrapHandshake)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto reader = createReader(createParams(List<IComponent>(sig1, sig2)));

    // The bootstrap re-emits descriptors of already-connected inputs: the first read is a handshake event
    auto status = readStatus(reader);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    auto descriptors = descriptorsOf(status);
    ASSERT_EQ(descriptors.getCount(), 2u);
    ASSERT_EQ(descriptors.get(sig1.getGlobalId()), sig1.getDescriptor());
    ASSERT_EQ(descriptors.get(sig2.getGlobalId()), sig2.getDescriptor());

    // Until committed the very same event is re-reported
    auto again = readStatus(reader);
    ASSERT_EQ(static_cast<IMultiReader2Status*>(again), static_cast<IMultiReader2Status*>(status));

    checkErrorInfo(reader->commitEvent());
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Data);
}

TEST_F(MultiReader2Test, DescriptorChangesCoalesceNewestWins)
{
    auto sig = createSignal("sig");
    auto reader = createReader(createParams(List<IComponent>(sig)));
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());

    auto descA = DataDescriptorBuilder().setSampleType(SampleType::Int32).build();
    auto descB = DataDescriptorBuilder().setSampleType(SampleType::Int16).build();
    sig.setDescriptor(descA);
    sig.setDescriptor(descB);

    auto status = readStatus(reader);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(descriptorsOf(status).get(sig.getGlobalId()), descB);
    checkErrorInfo(reader->commitEvent());
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Data);
}

TEST_F(MultiReader2Test, DisconnectParksThenWaitsSilently)
{
    auto sig = createSignal("sig");
    auto port = InputPort(context, nullptr, "port");
    port.connect(sig);
    auto reader = createReader(createParams(List<IComponent>(port)));

    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());

    port.disconnect();
    auto status = readStatus(reader);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    auto errors = errorsOf(status);
    ASSERT_EQ(errors.getCount(), 1u);
    ASSERT_EQ(static_cast<Int>(errors.get(port.getGlobalId())), static_cast<Int>(MultiReader2InputError::Disconnected));
    checkErrorInfo(reader->commitEvent());

    // A committed disconnect leaves the reader waiting silently for the reconnect
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Data);

    // The reconnect descriptor reopens the event window
    port.connect(sig);
    auto reconnect = readStatus(reader);
    ASSERT_EQ(typeOf(reconnect), MultiReader2StatusType::Event);
    ASSERT_EQ(errorsOf(reconnect).getCount(), 0u);
    checkErrorInfo(reader->commitEvent());
}

TEST_F(MultiReader2Test, UnusedInputEventsStillReported)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto params = createParams(List<IComponent>(sig1, sig2));
    checkErrorInfo(params->setUnusedInputs(List<IComponent>(sig2)));
    auto reader = createReader(params);

    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());

    auto newDesc = DataDescriptorBuilder().setSampleType(SampleType::Int8).build();
    sig2.setDescriptor(newDesc);
    auto status = readStatus(reader);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(descriptorsOf(status).get(sig2.getGlobalId()), newDesc);
}

TEST_F(MultiReader2Test, SyncEndToEnd)
{
    auto sig1 = createSignalWithDomain("sig1");
    auto sig2 = createSignalWithDomain("sig2");
    auto reader = createReader(createParams(List<IComponent>(sig1, sig2)));

    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());

    SizeT count = 99;
    checkErrorInfo(reader->getAvailableCount(&count));
    ASSERT_EQ(count, 0u);

    // Sync aligns both inputs to the latest start and only then samples count
    sendData(sig1, 100, 10);
    sendData(sig2, 104, 10);
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Data);
    checkErrorInfo(reader->getAvailableCount(&count));
    ASSERT_EQ(count, 6u);

    // A descriptor change parks, zeroes the count, and forces a resync after the commit
    // (Float64 width stays: sendData writes doubles into the packets)
    sig1.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float64).setName("changed").build());
    checkErrorInfo(reader->getAvailableCount(&count));
    ASSERT_EQ(count, 0u);
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());

    sendData(sig1, 120, 5);
    sendData(sig2, 120, 5);
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Data);
    checkErrorInfo(reader->getAvailableCount(&count));
    ASSERT_EQ(count, 5u);
}

TEST_F(MultiReader2Test, ReconfigureCancelsEventWindow)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto reader = createReader(createParams(List<IComponent>(sig1)));

    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);

    checkErrorInfo(reader->configure(createParams(List<IComponent>(sig2))));

    // The window died with the old state: gates are closed and a fresh handshake is pending
    ASSERT_EQ(reader->commitEvent(), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();
    auto status = readStatus(reader);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    auto descriptors = descriptorsOf(status);
    ASSERT_EQ(descriptors.getCount(), 1u);
    ASSERT_TRUE(descriptors.hasKey(sig2.getGlobalId()));
}

// --- Notification wiring (NullContext has no scheduler -> inline single-shot pass) ---

TEST_F(MultiReader2Test, DataAvailableFiresOnceUntilRearmed)
{
    auto sig = createSignal("sig");
    auto port = InputPort(context, nullptr, "port");
    auto reader = createReader(createParams(List<IComponent>(port)));

    std::atomic<int> fired{0};
    IEvent* eventIntf;
    checkErrorInfo(reader->getOnDataAvailable(&eventIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDataAvailable{ObjectPtr<IEvent>::Adopt(eventIntf)};
    onDataAvailable += [&fired](InputPortPtr&, EventArgsPtr<>&) { fired++; };

    // The connect descriptor is the first deliverable content and wins the only wake
    port.connect(sig);
    ASSERT_EQ(fired.load(), 1);

    // Disarmed until consumption: further packets do not fire
    sig.sendPacket(DataPacket(descriptor(), 10));
    ASSERT_EQ(fired.load(), 1);

    // Consuming the event re-arms; the next fresh packet wins a new pass
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());
    ASSERT_EQ(fired.load(), 1);
    sig.sendPacket(DataPacket(descriptor(), 10));
    ASSERT_EQ(fired.load(), 2);
}

TEST_F(MultiReader2Test, ConnectedEventCarriesSlotId)
{
    auto sig = createSignal("sig");
    auto port = InputPort(context, nullptr, "port");

    auto reader = createReader(createParams(List<IComponent>(port)));

    StringPtr connectedId;
    IEvent* connectedIntf;
    checkErrorInfo(reader->getOnConnected(&connectedIntf));
    Event<InputPortPtr, EventArgsPtr<>> onConnected{ObjectPtr<IEvent>::Adopt(connectedIntf)};
    onConnected += [&connectedId](InputPortPtr&, EventArgsPtr<>& args) { connectedId = args.getEventName(); };

    port.connect(sig);
    ASSERT_EQ(connectedId, port.getGlobalId());

    StringPtr disconnectedId;
    IEvent* disconnectedIntf;
    checkErrorInfo(reader->getOnDisconnected(&disconnectedIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDisconnected{ObjectPtr<IEvent>::Adopt(disconnectedIntf)};
    onDisconnected += [&disconnectedId](InputPortPtr&, EventArgsPtr<>& args) { disconnectedId = args.getEventName(); };

    port.disconnect();
    ASSERT_EQ(disconnectedId, port.getGlobalId());
}

TEST_F(MultiReader2Test, ReconfigureDuringConnectedCallback)
{
    auto sig = createSignal("sig");
    auto target = createSignal("target");
    auto port = InputPort(context, nullptr, "port");

    auto reader = createReader(createParams(List<IComponent>(port)));

    // Raw capture: a strong ref in the handler would cycle reader -> emitter -> handler -> reader
    IMultiReader2* readerRaw = reader;

    IEvent* connectedIntf;
    checkErrorInfo(reader->getOnConnected(&connectedIntf));
    Event<InputPortPtr, EventArgsPtr<>> onConnected{ObjectPtr<IEvent>::Adopt(connectedIntf)};
    onConnected += [readerRaw, &target](InputPortPtr&, EventArgsPtr<>&)
    {
        auto params = createParams(List<IComponent>(target));
        checkErrorInfo(readerRaw->configure(params));
    };

    // The handler reconfigures the reader away from the port while connect is still on the stack
    port.connect(sig);
    ASSERT_EQ(mainInputOf(reader), target.getGlobalId());
}

TEST_F(MultiReader2Test, ReconfigureDuringDisconnectedCallback)
{
    auto sig = createSignal("sig");
    auto target = createSignal("target");
    auto port = InputPort(context, nullptr, "port");
    port.connect(sig);

    auto reader = createReader(createParams(List<IComponent>(port)));

    IMultiReader2* readerRaw = reader;

    IEvent* disconnectedIntf;
    checkErrorInfo(reader->getOnDisconnected(&disconnectedIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDisconnected{ObjectPtr<IEvent>::Adopt(disconnectedIntf)};
    onDisconnected += [readerRaw, &target](InputPortPtr&, EventArgsPtr<>&)
    {
        auto params = createParams(List<IComponent>(target));
        checkErrorInfo(readerRaw->configure(params));
    };

    // detachSlot inside configure touches the very port whose disconnect is on the stack
    port.disconnect();
    ASSERT_EQ(mainInputOf(reader), target.getGlobalId());
}

TEST_F(MultiReader2Test, ReconfigureDuringDataAvailableCallback)
{
    auto sig = createSignal("sig");
    auto target = createSignal("target");
    auto reader = createReader(createParams(List<IComponent>(sig)));

    // Consume the bootstrap handshake so the next packet can win a wake
    ASSERT_EQ(typeOf(readStatus(reader)), MultiReader2StatusType::Event);
    checkErrorInfo(reader->commitEvent());

    IMultiReader2* readerRaw = reader;
    std::atomic<int> fired{0};

    IEvent* eventIntf;
    checkErrorInfo(reader->getOnDataAvailable(&eventIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDataAvailable{ObjectPtr<IEvent>::Adopt(eventIntf)};
    onDataAvailable += [readerRaw, &target, &fired](InputPortPtr&, EventArgsPtr<>&)
    {
        // Reconfigure only once: the bootstrap of the new input re-enters this handler inline
        if (fired++ > 0)
            return;
        auto params = createParams(List<IComponent>(target));
        checkErrorInfo(readerRaw->configure(params));
    };

    // Inline pass: the handler reconfigures away the signal whose sendPacket is still on the stack
    sig.sendPacket(DataPacket(descriptor(), 10));
    ASSERT_GE(fired.load(), 1);
    ASSERT_EQ(mainInputOf(reader), target.getGlobalId());
}

TEST_F(MultiReader2Test, WaitsForAllPortsConnected)
{
    auto sig1 = createSignal("sig1");
    auto sig2 = createSignal("sig2");
    auto port1 = InputPort(context, nullptr, "port1");
    auto port2 = InputPort(context, nullptr, "port2");

    auto reader = createReader(createParams(List<IComponent>(port1, port2)));

    std::atomic<int> fired{0};
    IEvent* eventIntf;
    checkErrorInfo(reader->getOnDataAvailable(&eventIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDataAvailable{ObjectPtr<IEvent>::Adopt(eventIntf)};
    onDataAvailable += [&fired](InputPortPtr&, EventArgsPtr<>&) { fired++; };

    // One connected port is not enough: its descriptor is cached but nothing fires
    port1.connect(sig1);
    ASSERT_EQ(fired.load(), 0);

    // The second connect completes the set; its initial descriptor event wakes the reader
    port2.connect(sig2);
    ASSERT_EQ(fired.load(), 1);
}

// --- Manager unit tests (plain C++, no ports involved) ---

class MultiReaderDataManagerTest : public testing::Test
{
protected:
    static MultiReaderDataManager::Config makeConfig(SizeT inputCount, std::vector<bool> usedFlags = {})
    {
        MultiReaderDataManager::Config config;
        for (SizeT i = 0; i < inputCount; i++)
            config.inputIds.push_back(String(("input" + std::to_string(i)).c_str()));
        config.usedFlags = std::move(usedFlags);
        config.mainInputId = config.inputIds[0];
        config.valueReadType = SampleType::Float64;
        return config;
    }

    static PacketPtr dataPacket(SizeT samples = 10)
    {
        return DataPacket(DataDescriptorBuilder().setSampleType(SampleType::Float64).build(), samples);
    }

    static PacketPtr eventPacket()
    {
        auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
        return DataDescriptorChangedEventPacket(desc, nullptr);
    }

    static DataDescriptorPtr valueDesc()
    {
        return DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    }

    static DataDescriptorPtr domainDesc(Int delta = 1, Int resNum = 1, Int resDen = 1000, const StringPtr& origin = "")
    {
        return DataDescriptorBuilder()
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s", -1, "second", "time"))
            .setTickResolution(Ratio(resNum, resDen))
            .setRule(LinearDataRule(delta, 0))
            .setOrigin(origin)
            .build();
    }

    static PacketPtr alignedPacket(const DataDescriptorPtr& value, const DataDescriptorPtr& domain, Int offset, SizeT samples)
    {
        return DataPacketWithDomain(DataPacket(domain, samples, offset), value, samples);
    }

    static SizeT availableOf(MultiReaderDataManager& manager)
    {
        SizeT count = 0;
        checkErrorInfo(manager.getAvailableCount(&count));
        return count;
    }

    static Int errorOf(const ObjectPtr<IMultiReader2Status>& status, const char* inputId)
    {
        return errorsOf(status).get(String(inputId));
    }
};

TEST_F(MultiReaderDataManagerTest, WakesWhenAllUsedInputsHaveData)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    ASSERT_FALSE(manager.addPacket(0, dataPacket()));
    ASSERT_TRUE(manager.addPacket(1, dataPacket()));
}

TEST_F(MultiReaderDataManagerTest, EventPacketAlwaysWakes)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    ASSERT_TRUE(manager.addPacket(0, eventPacket()));
}

TEST_F(MultiReaderDataManagerTest, UnusedInputDropsDataButReportsEvents)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2, {true, false}));

    // Data on the unused input is dropped and never wakes
    ASSERT_FALSE(manager.addPacket(1, dataPacket()));
    // The used input alone satisfies the all-ready condition
    ASSERT_TRUE(manager.addPacket(0, dataPacket()));

    manager.reconfigure(makeConfig(2, {true, false}));
    // Events on the unused input still wake
    ASSERT_TRUE(manager.addPacket(1, eventPacket()));
}

TEST_F(MultiReaderDataManagerTest, WakeElectionFiresOnce)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(1));

    ASSERT_TRUE(manager.addPacket(0, dataPacket()));
    ASSERT_FALSE(manager.addPacket(0, dataPacket()));
    ASSERT_FALSE(manager.addPacket(0, dataPacket()));

    // Still deliverable: the pass is owed again, the manager stays disarmed
    ASSERT_TRUE(manager.armDataAvailable());
    ASSERT_FALSE(manager.addPacket(0, dataPacket()));
}

TEST_F(MultiReaderDataManagerTest, ReadReportsEventThenData)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(1));

    ASSERT_TRUE(manager.addPacket(0, eventPacket()));

    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(static_cast<IMultiReader2Status*>(readStatus(manager)), static_cast<IMultiReader2Status*>(status));

    checkErrorInfo(manager.commitEvent());
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
}

TEST_F(MultiReaderDataManagerTest, PartialEventsMergeFullState)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(1));

    auto valueDesc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    auto domainDesc1 = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    auto domainDesc2 = DataDescriptorBuilder().setSampleType(SampleType::UInt64).build();

    manager.addPacket(0, DataDescriptorChangedEventPacket(valueDesc, domainDesc1));
    manager.addPacket(0, DataDescriptorChangedEventPacket(nullptr, domainDesc2));

    // The value descriptor from the first delta survives the newest-wins cache
    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(descriptorsOf(status).get(String("input0")), valueDesc);

    IDataDescriptor* domainRaw;
    checkErrorInfo(status->getDomainDescriptor(&domainRaw));
    ASSERT_EQ(DataDescriptorPtr::Adopt(domainRaw), domainDesc2);
}

TEST_F(MultiReaderDataManagerTest, SetUsedDuringDisconnectWindowUnblocks)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    manager.disconnected(1);
    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_TRUE(errorsOf(status).hasKey(String("input1")));

    checkErrorInfo(manager.setUsed(String("input1"), False));
    checkErrorInfo(manager.commitEvent());

    // The remaining used input alone drives the wake again
    ASSERT_TRUE(manager.addPacket(0, dataPacket()));
}

TEST_F(MultiReaderDataManagerTest, AvailableCountMinOverUsedInputs)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2, {true, false}));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    // Nothing counts before sync completes
    ASSERT_EQ(availableOf(manager), 0u);

    // The main input syncs to itself; the unused input never participates
    manager.addPacket(0, alignedPacket(value, domain, 0, 7));
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
    ASSERT_EQ(availableOf(manager), 7u);

    // Data under a foreign descriptor does not count
    manager.addPacket(0, DataPacket(DataDescriptorBuilder().setSampleType(SampleType::Int32).build(), 5));
    ASSERT_EQ(availableOf(manager), 7u);

    // A parked event zeroes the count
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    ASSERT_EQ(availableOf(manager), 0u);
}

// --- Synchronization ---

TEST_F(MultiReaderDataManagerTest, SyncAlignsToLatestStart)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domain));
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Event);
    checkErrorInfo(manager.commitEvent());

    manager.addPacket(0, alignedPacket(value, domain, 100, 10));
    manager.addPacket(1, alignedPacket(value, domain, 105, 10));

    // Samples below the latest start are discarded and sync completes within the read
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
    ASSERT_EQ(availableOf(manager), 5u);
}

TEST_F(MultiReaderDataManagerTest, SyncFailsMismatchedDomains)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(3));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domainDesc(2)));
    manager.addPacket(2, DataDescriptorChangedEventPacket(value, domainDesc(1, 1, 1000, "1970-01-01T00:00:00")));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    // A wrong rate and a wrong origin both break the relation to the main input
    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorOf(status, "input1"), static_cast<Int>(MultiReader2InputError::InvalidDomain));
    ASSERT_EQ(errorOf(status, "input2"), static_cast<Int>(MultiReader2InputError::InvalidDomain));
    checkErrorInfo(manager.commitEvent());

    // The failed inputs sit out: the main input alone syncs and counts
    manager.addPacket(0, alignedPacket(value, domain, 0, 4));
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
    ASSERT_EQ(availableOf(manager), 4u);
}

TEST_F(MultiReaderDataManagerTest, SyncScalesEqualRateResolutions)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    // delta=2 at resolution 1/2000 is the same rate as delta=1 at 1/1000
    auto value = valueDesc();
    auto domainMain = domainDesc(1, 1, 1000);
    auto domainHalfTicks = domainDesc(2, 1, 2000);
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domainMain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domainHalfTicks));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    // Main [100..109] ms; the second input starts at tick 210 = 105 ms
    manager.addPacket(0, alignedPacket(value, domainMain, 100, 10));
    manager.addPacket(1, alignedPacket(value, domainHalfTicks, 210, 10));

    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
    ASSERT_EQ(availableOf(manager), 5u);
}

TEST_F(MultiReaderDataManagerTest, SyncFailsOffLatticeTicks)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    auto value = valueDesc();
    auto domainMain = domainDesc(1, 1, 1000);
    auto domainHalfTicks = domainDesc(2, 1, 2000);
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domainMain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domainHalfTicks));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    // Tick 209 is 104.5 ms: off the main lattice for good
    manager.addPacket(0, alignedPacket(value, domainMain, 100, 10));
    manager.addPacket(1, alignedPacket(value, domainHalfTicks, 209, 10));

    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorOf(status, "input1"), static_cast<Int>(MultiReader2InputError::InvalidDomain));
}

TEST_F(MultiReaderDataManagerTest, RejectsNonIntegerSampleRate)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    // 1000 ticks per second at delta 3 is not a whole number of samples per second
    auto value = valueDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domainDesc(1, 1, 1000)));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domainDesc(3, 1, 1000)));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorOf(status, "input1"), static_cast<Int>(MultiReader2InputError::InvalidDomain));
}

TEST_F(MultiReaderDataManagerTest, SyncFailsOffGridInput)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    auto value = valueDesc();
    auto domain = domainDesc(2);
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    manager.addPacket(0, alignedPacket(value, domain, 100, 5));
    manager.addPacket(1, alignedPacket(value, domain, 101, 5));

    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorOf(status, "input1"), static_cast<Int>(MultiReader2InputError::InvalidDomain));
}

TEST_F(MultiReaderDataManagerTest, SyncFailsDistantRanges)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    // 1/1000 resolution makes the 2 second window 2000 ticks; these ranges sit 4891 apart
    manager.addPacket(0, alignedPacket(value, domain, 5000, 10));
    manager.addPacket(1, alignedPacket(value, domain, 100, 10));

    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorOf(status, "input1"), static_cast<Int>(MultiReader2InputError::SyncFailed));
}

TEST_F(MultiReaderDataManagerTest, SyncTimesOutSilentInput)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    manager.addPacket(0, alignedPacket(value, domain, 100, 10));
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);

    // The silent input fails once the 2 second window closes; the rest sync without it.
    // Past the deadline a flowing input wakes the consumer even though not all inputs are ready.
    manager.armDataAvailable();
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));
    ASSERT_TRUE(manager.addPacket(0, alignedPacket(value, domain, 110, 5)));
    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorOf(status, "input1"), static_cast<Int>(MultiReader2InputError::SyncFailed));
    checkErrorInfo(manager.commitEvent());

    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
    ASSERT_EQ(availableOf(manager), 15u);
}

TEST_F(MultiReaderDataManagerTest, EventDuringSyncFailsInputUntilCommit)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    // Sync starts on the main data and still waits for the second input
    manager.addPacket(0, alignedPacket(value, domain, 100, 10));
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);

    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domain));
    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorOf(status, "input1"), static_cast<Int>(MultiReader2InputError::SyncFailed));
    checkErrorInfo(manager.commitEvent());

    // The fresh descriptor lets the input rejoin and sync completes
    manager.addPacket(1, alignedPacket(value, domain, 100, 10));
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
    ASSERT_EQ(availableOf(manager), 10u);
}

TEST_F(MultiReaderDataManagerTest, ReadConvertsAndAdvances)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    auto value = DataDescriptorBuilder().setSampleType(SampleType::Int32).build();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    manager.addPacket(1, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    const auto sendInt = [&](SizeT slot, Int offset, SizeT samples)
    {
        auto packet = DataPacketWithDomain(DataPacket(domain, samples, offset), value, samples);
        const auto out = static_cast<int32_t*>(packet.getData());
        for (SizeT i = 0; i < samples; i++)
            out[i] = static_cast<int32_t>(offset) + static_cast<int32_t>(i);
        manager.addPacket(slot, packet);
    };
    sendInt(0, 100, 10);
    sendInt(1, 105, 10);

    double b0[8] = {};
    double b1[8] = {};
    void* buffers[2] = {b0, b1};
    IMultiReader2Status* statusRaw;
    SizeT count = 4;
    SizeT offset = 0;
    checkErrorInfo(manager.read(&statusRaw, buffers, &count, &offset));
    auto status = ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);

    // Int32 samples arrive as doubles, aligned to the latest start
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Data);
    ASSERT_EQ(count, 4u);
    ASSERT_EQ(offset, 105u);
    ASSERT_DOUBLE_EQ(b0[0], 105.0);
    ASSERT_DOUBLE_EQ(b1[0], 105.0);
    ASSERT_DOUBLE_EQ(b0[3], 108.0);

    // The next read continues where the last one stopped
    count = 8;
    checkErrorInfo(manager.read(&statusRaw, buffers, &count, &offset));
    status = ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);
    ASSERT_EQ(count, 1u);
    ASSERT_EQ(offset, 109u);
    ASSERT_DOUBLE_EQ(b0[0], 109.0);
}

TEST_F(MultiReaderDataManagerTest, MinReadCountGatesReads)
{
    MultiReaderDataManager manager;
    auto config = makeConfig(1);
    config.minReadCount = 5;
    manager.reconfigure(std::move(config));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());
    manager.addPacket(0, alignedPacket(value, domain, 100, 4));

    double buffer[16] = {};
    void* buffers[1] = {buffer};
    IMultiReader2Status* statusRaw;
    SizeT count = 3;
    SizeT offset = 0;
    ASSERT_EQ(manager.read(&statusRaw, buffers, &count, &offset), OPENDAQ_ERR_INVALIDPARAMETER);
    daqClearErrorInfo();

    // Below the minimum nothing is returned; enough data satisfies the whole request at once
    count = 16;
    checkErrorInfo(manager.read(&statusRaw, buffers, &count, &offset));
    ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);
    ASSERT_EQ(count, 0u);

    manager.addPacket(0, alignedPacket(value, domain, 104, 3));
    count = 16;
    checkErrorInfo(manager.read(&statusRaw, buffers, &count, &offset));
    ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);
    ASSERT_EQ(count, 7u);
}

TEST_F(MultiReaderDataManagerTest, GapParksAndResyncs)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(1));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    manager.addPacket(0, alignedPacket(value, domain, 100, 5));
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
    ASSERT_EQ(availableOf(manager), 5u);

    // The gap parks with a Gap error and sacrifices the pre-gap samples
    manager.armDataAvailable();
    ASSERT_TRUE(manager.addPacket(0, ImplicitDomainGapDetectedEventPacket(3)));
    auto status = readStatus(manager);
    ASSERT_EQ(typeOf(status), MultiReader2StatusType::Event);
    ASSERT_EQ(errorOf(status, "input0"), static_cast<Int>(MultiReader2InputError::Gap));
    checkErrorInfo(manager.commitEvent());

    // Post-gap data realigns through the usual resync
    manager.addPacket(0, alignedPacket(value, domain, 200, 5));
    ASSERT_EQ(typeOf(readStatus(manager)), MultiReader2StatusType::Data);
    ASSERT_EQ(availableOf(manager), 5u);
}

TEST_F(MultiReaderDataManagerTest, ReadWithDomainTimestampsInMainTicks)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(1));

    auto value = valueDesc();
    auto domain = domainDesc();
    manager.addPacket(0, DataDescriptorChangedEventPacket(value, domain));
    readStatus(manager);
    checkErrorInfo(manager.commitEvent());

    auto packet = DataPacketWithDomain(DataPacket(domain, 5, 50), value, 5);
    const auto out = static_cast<double*>(packet.getData());
    for (SizeT i = 0; i < 5; i++)
        out[i] = 50.0 + static_cast<double>(i);
    manager.addPacket(0, packet);

    int64_t timestamps[8] = {};
    double values[8] = {};
    void* buffers[2] = {timestamps, values};
    IMultiReader2Status* statusRaw;
    SizeT count = 8;
    checkErrorInfo(manager.readWithDomain(&statusRaw, buffers, &count));
    ObjectPtr<IMultiReader2Status>::Adopt(statusRaw);
    ASSERT_EQ(count, 5u);
    ASSERT_EQ(timestamps[0], 50);
    ASSERT_EQ(timestamps[4], 54);
    ASSERT_DOUBLE_EQ(values[3], 53.0);
}

TEST_F(MultiReaderDataManagerTest, RejectsOutOfRangeSlots)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(1));

    ASSERT_FALSE(manager.addPacket(5, dataPacket()));
    ASSERT_FALSE(manager.addPacket(0, nullptr));
}

TEST_F(MultiReaderDataManagerTest, GatesRequireEventWindow)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(1));

    ASSERT_EQ(manager.commitEvent(), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();
    ASSERT_EQ(manager.setActive(False), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();
    ASSERT_EQ(manager.setUsed(String("input0"), False), OPENDAQ_ERR_INVALIDSTATE);
    daqClearErrorInfo();
}

TEST_F(MultiReaderDataManagerTest, ReconfigureResetsQueues)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(1));

    ASSERT_TRUE(manager.addPacket(0, dataPacket()));
    manager.reconfigure(makeConfig(1));

    // Fresh state: armed again, queue empty, so one packet wakes again
    ASSERT_TRUE(manager.addPacket(0, dataPacket()));
}

TEST_F(MultiReaderDataManagerTest, DisconnectedUsedInputSilencesEverything)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    manager.disconnected(0);

    // All packets on all inputs are dropped, events included
    ASSERT_FALSE(manager.addPacket(0, eventPacket()));
    ASSERT_FALSE(manager.addPacket(1, dataPacket()));
    ASSERT_FALSE(manager.addPacket(1, eventPacket()));

    // Full reconnection restores waking
    manager.connected(0);
    ASSERT_TRUE(manager.addPacket(0, eventPacket()));
}

TEST_F(MultiReaderDataManagerTest, DisconnectedUnusedInputDoesNotBlock)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2, {true, false}));

    manager.disconnected(1);
    ASSERT_TRUE(manager.addPacket(0, dataPacket()));
}

TEST_F(MultiReaderDataManagerTest, ConcurrentProducersElectOneWake)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    constexpr int packetsPerProducer = 500;
    std::atomic<int> wakes{0};

    auto producer = [&](SizeT slot)
    {
        for (int i = 0; i < packetsPerProducer; i++)
        {
            if (manager.addPacket(slot, dataPacket(1)))
                wakes++;
        }
    };

    std::thread t0(producer, 0);
    std::thread t1(producer, 1);
    t0.join();
    t1.join();

    // Once both slots ever held data the condition stays true, so exactly one wake can be won
    ASSERT_EQ(wakes.load(), 1);
}

TEST_F(MultiReaderDataManagerTest, SurvivesReconfigureUnderFire)
{
    MultiReaderDataManager manager;
    manager.reconfigure(makeConfig(2));

    std::atomic<bool> stop{false};
    auto producer = [&](SizeT slot)
    {
        while (!stop.load())
            manager.addPacket(slot, dataPacket(1));
    };

    std::thread t0(producer, 0);
    std::thread t1(producer, 1);

    for (int i = 0; i < 50; i++)
        manager.reconfigure(makeConfig(2));

    stop = true;
    t0.join();
    t1.join();
}
