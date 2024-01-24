#include <opcuatms/exceptions.h>
#include "test_helpers.h"

using NativeDeviceModulesTest = testing::Test;

using namespace daq;

static InstancePtr CreateServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto typeManager = TypeManager();
    auto context = Context(scheduler, logger, typeManager, moduleManager);

    auto instance = InstanceCustom(context, "local");

    const auto statistics = instance.addFunctionBlock("ref_fb_module_statistics");
    const auto refDevice = instance.addDevice("daqref://device1");
    statistics.getInputPorts()[0].connect(refDevice.getSignals(search::Recursive(search::Visible()))[0]);
    statistics.getInputPorts()[0].connect(Signal(context, nullptr, "foo"));

    instance.addServer("openDAQ Native Streaming", nullptr);

    return instance;
}

static InstancePtr CreateClientInstance()
{
    auto instance = Instance();

    auto refDevice = instance.addDevice("daq.ncd://127.0.0.1", nullptr);
    return instance;
}

TEST_F(NativeDeviceModulesTest, ConnectAndDisconnect)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    client->releaseRef();
    server->releaseRef();
    client.detach();
    server.detach();
}

TEST_F(NativeDeviceModulesTest, GetRemoteDeviceObjects)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignals(search::Recursive(search::Any()));
    auto signalsServer = server.getSignals(search::Recursive(search::Any()));
    ASSERT_EQ(signals.getCount(), 7u);
    auto signalsVisible = client.getSignals(search::Recursive(search::Visible()));
    ASSERT_EQ(signalsVisible.getCount(), 4u);
    auto devices = client.getDevices();
    ASSERT_EQ(devices.getCount(), 1u);
    auto fbs = devices[0].getFunctionBlocks();
    ASSERT_EQ(fbs.getCount(), 1u);
    auto channels = client.getChannels(search::Recursive(search::Any()));
    ASSERT_EQ(channels.getCount(), 2u);
}

TEST_F(NativeDeviceModulesTest, RemoteGlobalIds)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    const auto clientRootId = client.getGlobalId();
    const auto serverDeviceId = server.getGlobalId();
    const auto clientDeviceId = client.getDevices()[0].getGlobalId();

    const auto serverSignalId = server.getSignalsRecursive()[0].getGlobalId();
    const auto clientSignalId = client.getSignalsRecursive()[0].getGlobalId();

    ASSERT_EQ(clientDeviceId, clientRootId + "/Dev" + serverDeviceId);
    ASSERT_EQ(clientSignalId, clientRootId + "/Dev" + serverSignalId);
}

TEST_F(NativeDeviceModulesTest, GetSetDeviceProperties)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto refDevice = client.getDevices()[0].getDevices()[0];
    auto serverRefDevice = server.getDevices()[0];

    PropertyPtr propInfo;
    ASSERT_NO_THROW(propInfo = refDevice.getProperty("NumberOfChannels"));
    ASSERT_EQ(propInfo.getValueType(), CoreType::ctInt);

    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 2);
    refDevice.setPropertyValue("NumberOfChannels", 3);
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 3);
    ASSERT_EQ(serverRefDevice.getPropertyValue("NumberOfChannels"), 3);

    refDevice.setPropertyValue("NumberOfChannels", 1);
    ASSERT_EQ(refDevice.getPropertyValue("NumberOfChannels"), 1);
    ASSERT_EQ(serverRefDevice.getPropertyValue("NumberOfChannels"), 1);

    refDevice.setPropertyValue("GlobalSampleRate", 2000);
    ASSERT_EQ(refDevice.getPropertyValue("GlobalSampleRate"), 2000);
    ASSERT_EQ(serverRefDevice.getPropertyValue("GlobalSampleRate"), 2000);

    ASSERT_ANY_THROW(refDevice.setPropertyValue("InvalidProp", 100));

    auto properties = refDevice.getAllProperties();
    ASSERT_EQ(properties.getCount(), 5u);
}

TEST_F(NativeDeviceModulesTest, DeviceInfo)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto info = client.getDevices()[0].getInfo();

    ASSERT_TRUE(info.assigned());
    ASSERT_EQ(info.getConnectionString(), "daq.ncd://127.0.0.1");
}

TEST_F(NativeDeviceModulesTest, ChannelProps)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
    auto dev = client.getDevices()[0].getDevices()[0];
    auto customRangeValue = dev.getChannels()[0].getPropertyValue("CustomRange").asPtr<IStruct>();

    ASSERT_EQ(customRangeValue.get("lowValue"), -10.0);
    ASSERT_EQ(customRangeValue.get("highValue"), 10.0);
}

TEST_F(NativeDeviceModulesTest, FunctionBlockProperties)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto fb = client.getDevices()[0].getFunctionBlocks()[0];
    auto serverFb = server.getFunctionBlocks()[0];

    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));
    fb.setPropertyValue("BlockSize", 20);
    ASSERT_EQ(fb.getPropertyValue("BlockSize"), serverFb.getPropertyValue("BlockSize"));

    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));
    fb.setPropertyValue("DomainSignalType", 2);
    ASSERT_EQ(fb.getPropertyValue("DomainSignalType"), serverFb.getPropertyValue("DomainSignalType"));

    ASSERT_ANY_THROW(fb.setPropertyValue("DomainSignalType", 1000));
}

TEST_F(NativeDeviceModulesTest, ProcedureProp)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto ch = client.getDevices()[0].getDevices()[0].getChannels()[0];
    ch.setPropertyValue("Waveform", 3);
    ProcedurePtr proc = ch.getPropertyValue("ResetCounter");
    ASSERT_NO_THROW(proc());
}

TEST_F(NativeDeviceModulesTest, SignalDescriptors)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    DataDescriptorPtr dataDescriptor = client.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
    DataDescriptorPtr serverDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[0].getDescriptor();

    DataDescriptorPtr domainDataDescriptor = client.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();
    DataDescriptorPtr serverDomainDataDescriptor = server.getSignals(search::Recursive(search::Visible()))[2].getDescriptor();

    ASSERT_TRUE(dataDescriptor.assigned());
    ASSERT_TRUE(domainDataDescriptor.assigned());

    ASSERT_EQ(dataDescriptor, serverDataDescriptor);
    ASSERT_EQ(domainDataDescriptor, serverDomainDataDescriptor);

//    auto refChannel = client.getChannels(search::Recursive(search::Visible()))[0];
//    refChannel.setPropertyValue("ClientSideScaling", true);

//    dataDescriptor = client.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
//    serverDataDescriptor = server.getChannels(search::Recursive(search::Visible()))[0].getSignals(search::Recursive(search::Visible()))[0].getDescriptor();
//    ASSERT_EQ(dataDescriptor.getPostScaling().getParameters(), dataDescriptor.getPostScaling().getParameters());
}

TEST_F(NativeDeviceModulesTest, SubscribeReadUnsubscribe)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto device = client.getDevices()[0].getDevices()[0];
    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_TRUE(deviceSignal0.getDomainSignal().assigned());

    auto signal = deviceSignal0.template asPtr<IMirroredSignalConfig>();
    auto domainSignal = deviceSignal0.getDomainSignal().template asPtr<IMirroredSignalConfig>();

    StringPtr streamingSource = signal.getActiveStreamingSource();
    ASSERT_EQ(streamingSource, domainSignal.getActiveStreamingSource());

    std::promise<StringPtr> signalSubscribePromise;
    std::future<StringPtr> signalSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(signalSubscribePromise, signalSubscribeFuture, signal);

    std::promise<StringPtr> domainSubscribePromise;
    std::future<StringPtr> domainSubscribeFuture;
    test_helpers::setupSubscribeAckHandler(domainSubscribePromise, domainSubscribeFuture, domainSignal);

    std::promise<StringPtr> signalUnsubscribePromise;
    std::future<StringPtr> signalUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(signalUnsubscribePromise, signalUnsubscribeFuture, signal);

    std::promise<StringPtr> domainUnsubscribePromise;
    std::future<StringPtr> domainUnsubscribeFuture;
    test_helpers::setupUnsubscribeAckHandler(domainUnsubscribePromise, domainUnsubscribeFuture, domainSignal);

    using namespace std::chrono_literals;
    StreamReaderPtr reader = daq::StreamReader<double, uint64_t>(signal);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalSubscribeFuture));
    ASSERT_EQ(signalSubscribeFuture.get(), streamingSource);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainSubscribeFuture));
    ASSERT_EQ(domainSubscribeFuture.get(), streamingSource);

    double samples[100];
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(100ms);
        daq::SizeT count = 100;
        reader.read(samples, &count);
        EXPECT_GT(count, 0) << "iteration " << i;
    }

    reader.release();

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(signalUnsubscribeFuture));
    ASSERT_EQ(signalUnsubscribeFuture.get(), streamingSource);

    ASSERT_TRUE(test_helpers::waitForAcknowledgement(domainUnsubscribeFuture));
    ASSERT_EQ(domainUnsubscribeFuture.get(), streamingSource);
}

TEST_F(NativeDeviceModulesTest, DISABLED_RendererSimple)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto device = client.getDevices()[0].getDevices()[0];
    const auto deviceChannel0 = device.getChannels()[0];
    const auto deviceSignal0 = deviceChannel0.getSignals(search::Recursive(search::Visible()))[0];

    ASSERT_TRUE(deviceSignal0.getDomainSignal().assigned());

    const auto rendererFb = client.addFunctionBlock("ref_fb_module_renderer");
    const auto rendererInputPort0 = rendererFb.getInputPorts()[0];
    rendererInputPort0.connect(deviceSignal0);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
