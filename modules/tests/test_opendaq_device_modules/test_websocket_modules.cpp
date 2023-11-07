#include <testutils/testutils.h>
#include <opendaq/opendaq.h>
#include "testutils/memcheck_listener.h"
#include <thread>

using WebsocketModulesTest = testing::Test;

// MAC CI issue
#if !defined(SKIP_TEST_MAC_CI)
    #if defined(__clang__)
        #define SKIP_TEST_MAC_CI return
    #else
        #define SKIP_TEST_MAC_CI
    #endif
#endif
using namespace daq;

static InstancePtr CreateServerInstance()
{
    auto logger = Logger();
    auto scheduler = Scheduler(logger);
    auto moduleManager = ModuleManager("");
    auto context = Context(scheduler, logger, nullptr, moduleManager);

    auto instance = InstanceCustom(context, "local");

    const auto statistics = instance.addFunctionBlock("ref_fb_module_statistics");
    const auto refDevice = instance.addDevice("daqref://device1");
    statistics.getInputPorts()[0].connect(refDevice.getSignalsRecursive()[0]);

    instance.addServer("openDAQ WebsocketTcp Streaming", nullptr);

    return instance;
}

static InstancePtr CreateClientInstance()
{
    auto instance = Instance();
    auto refDevice = instance.addDevice("daq.ws://127.0.0.1/");
    return instance;
}

TEST_F(WebsocketModulesTest, ConnectFail)
{
    SKIP_TEST_MAC_CI;
    ASSERT_THROW(CreateClientInstance(), NotFoundException);
}

TEST_F(WebsocketModulesTest, ConnectAndDisconnect)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();
}

TEST_F(WebsocketModulesTest, GetRemoteDeviceObjects)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto signals = client.getSignalsRecursive();
    ASSERT_EQ(signals.getCount(), 7u);
}

TEST_F(WebsocketModulesTest, SignalConfig_Server)
{
    SKIP_TEST_MAC_CI;
    const std::string newSignalName{"some new name"};

    auto server = CreateServerInstance();

    auto serverSignal = server.getSignalsRecursive()[0].asPtr<ISignalConfig>();
    auto serverSignalDataDescriptor = DataDescriptorBuilderCopy(serverSignal.getDescriptor()).setName(newSignalName).build();
    serverSignal.setDescriptor(serverSignalDataDescriptor);

    auto client = CreateClientInstance();

    auto clientSignals = client.getDevices()[0].getSignalsRecursive();
    auto clientSignal = clientSignals[0].asPtr<ISignalConfig>();

    auto clientSignalDataDescriptor = DataDescriptorBuilderCopy(clientSignal.getDescriptor()).build();

    ASSERT_EQ(serverSignal.getDescriptor().getName(), newSignalName);
    ASSERT_EQ(serverSignal.getDescriptor().getName(), clientSignal.getDescriptor().getName());
}

TEST_F(WebsocketModulesTest, DataDescriptor)
{
    SKIP_TEST_MAC_CI;
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    DataDescriptorPtr dataDescriptor = client.getSignalsRecursive()[0].getDescriptor();
    DataDescriptorPtr serverDataDescriptor = server.getSignalsRecursive()[0].getDescriptor();

    DataDescriptorPtr domainDataDescriptor = client.getSignalsRecursive()[1].getDescriptor();
    DataDescriptorPtr serverDomainDataDescriptor = server.getSignalsRecursive()[1].getDescriptor();

    ASSERT_EQ(dataDescriptor.getName(), serverDataDescriptor.getName());
    ASSERT_EQ(dataDescriptor.getDimensions().getCount(), serverDataDescriptor.getDimensions().getCount());

    ASSERT_EQ(dataDescriptor.getSampleType(), serverDataDescriptor.getSampleType());
    ASSERT_EQ(dataDescriptor.getUnit().getSymbol(), serverDataDescriptor.getUnit().getSymbol());
    ASSERT_EQ(dataDescriptor.getValueRange(), serverDataDescriptor.getValueRange());
    ASSERT_EQ(dataDescriptor.getRule().getType(), serverDataDescriptor.getRule().getType());
    ASSERT_EQ(dataDescriptor.getName(), serverDataDescriptor.getName());
    ASSERT_EQ(dataDescriptor.getMetadata(), serverDataDescriptor.getMetadata());

    ASSERT_EQ(domainDataDescriptor.getRule().getParameters(), serverDomainDataDescriptor.getRule().getParameters());
    ASSERT_EQ(domainDataDescriptor.getOrigin(), serverDomainDataDescriptor.getOrigin());
    ASSERT_EQ(domainDataDescriptor.getTickResolution(), serverDomainDataDescriptor.getTickResolution());
}

TEST_F(WebsocketModulesTest, DISABLED_RenderSignal)
{
    auto server = CreateServerInstance();
    auto client = CreateClientInstance();

    auto signals = client.getSignalsRecursive();
    const auto renderer = client.addFunctionBlock("ref_fb_module_renderer");
    renderer.getInputPorts()[0].connect(signals[0]);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
