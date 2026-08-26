#include <opendaq/context_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_params_impl.h>
#include <opendaq/multi_reader_data_manager.h>
#include <opendaq/packet_factory.h>
#include <opendaq/signal_factory.h>

#include <coretypes/event_wrapper.h>

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

using namespace daq;

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

TEST_F(MultiReader2Test, ReadShells)
{
    auto sig = createSignal("sig");
    auto reader = createReader(createParams(List<IComponent>(sig)));

    SizeT count = 0;
    ASSERT_EQ(reader->getAvailableCount(&count), OPENDAQ_SUCCESS);
    ASSERT_EQ(count, 0u);

    IMultiReader2Status* status;
    void* data[1] = {nullptr};
    SizeT readCount = 0;
    SizeT offset = 0;
    ASSERT_EQ(reader->read(&status, data, &readCount, &offset), OPENDAQ_ERR_NOTIMPLEMENTED);
    daqClearErrorInfo();
}

// --- Notification wiring (NullContext has no scheduler -> inline single-shot pass) ---

TEST_F(MultiReader2Test, DataAvailableFiresOnceUntilRearmed)
{
    auto sig = createSignal("sig");
    auto reader = createReader(createParams(List<IComponent>(sig)));

    std::atomic<int> fired{0};
    IEvent* eventIntf;
    checkErrorInfo(reader->getOnDataAvailable(&eventIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDataAvailable{ObjectPtr<IEvent>::Adopt(eventIntf)};
    onDataAvailable += [&fired](InputPortPtr&, EventArgsPtr<>&) { fired++; };

    // The initial descriptor event is drained on the first send and makes the reader deliverable
    sig.sendPacket(DataPacket(descriptor(), 10));
    ASSERT_EQ(fired.load(), 1);

    // Disarmed until consumption exists: further packets do not fire
    sig.sendPacket(DataPacket(descriptor(), 10));
    ASSERT_EQ(fired.load(), 1);
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

    IMultiReader2* readerRaw = reader;
    std::atomic<int> fired{0};

    IEvent* eventIntf;
    checkErrorInfo(reader->getOnDataAvailable(&eventIntf));
    Event<InputPortPtr, EventArgsPtr<>> onDataAvailable{ObjectPtr<IEvent>::Adopt(eventIntf)};
    onDataAvailable += [readerRaw, &target, &fired](InputPortPtr&, EventArgsPtr<>&)
    {
        fired++;
        auto params = createParams(List<IComponent>(target));
        checkErrorInfo(readerRaw->configure(params));
    };

    // Inline pass: the handler reconfigures away the signal whose sendPacket is still on the stack
    sig.sendPacket(DataPacket(descriptor(), 10));
    ASSERT_EQ(fired.load(), 1);
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

    // One connected port is not enough: its initial descriptor event is dropped, nothing fires
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
