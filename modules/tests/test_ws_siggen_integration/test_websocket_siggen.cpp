#include <testutils/testutils.h>
#include <opendaq/opendaq.h>
#include "testutils/memcheck_listener.h"
#include <thread>

using WebsocketSiggenTest = testing::Test;

using namespace daq;

static InstancePtr CreateClientInstance()
{
    auto instance = Instance();
    auto refDevice = instance.addDevice("daq.ws://127.0.0.1:7413/");
    return instance;
}

// signals configuration is set by "siggen_config.json"

TEST_F(WebsocketSiggenTest, ConnectAndDisconnect)
{
    auto client = CreateClientInstance();
}

TEST_F(WebsocketSiggenTest, GetRemoteDeviceObjects)
{
    auto client = CreateClientInstance();

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto signals = client.getSignalsRecursive();
    ASSERT_EQ(signals.getCount(), 2u);
}

TEST_F(WebsocketSiggenTest, SyncSignalDescriptors)
{
    auto client = CreateClientInstance();

    auto signal  = client.getSignalsRecursive()[0];

    EXPECT_EQ(signal.getLocalId(), "Signal1_Sync");

    DataDescriptorPtr dataDescriptor = signal.getDescriptor();
    DataDescriptorPtr domainDescriptor = signal.getDomainSignal().getDescriptor();

    EXPECT_EQ(dataDescriptor.getName(), "value");

    EXPECT_EQ(dataDescriptor.getSampleType(), SampleType::Float64);
    EXPECT_EQ(dataDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(dataDescriptor.getValueRange(), Range(-15.0, 15.0)); // default set by openDAQ client

    EXPECT_EQ(dataDescriptor.getDimensions().getCount(), 0u);
    EXPECT_EQ(dataDescriptor.getMetadata().getCount(), 0u);
    EXPECT_FALSE(dataDescriptor.getUnit().assigned());

    EXPECT_EQ(domainDescriptor.getName(), "time");
    EXPECT_EQ(domainDescriptor.getRule().getType(), DataRuleType::Linear);
    EXPECT_EQ(domainDescriptor.getUnit().getSymbol(), "s");
    EXPECT_EQ(domainDescriptor.getUnit().getQuantity(), "time");
    EXPECT_NE(domainDescriptor.getOrigin(), "");
    EXPECT_NE(domainDescriptor.getTickResolution().getNumerator(), 0);
    EXPECT_NE(domainDescriptor.getTickResolution().getDenominator(), 0);
}

TEST_F(WebsocketSiggenTest, AsyncSignalDescriptors)
{
    auto client = CreateClientInstance();

    auto signal  = client.getSignalsRecursive()[1];

    EXPECT_EQ(signal.getLocalId(), "Signal2_Async");

    DataDescriptorPtr dataDescriptor = signal.getDescriptor();
    DataDescriptorPtr domainDescriptor = signal.getDomainSignal().getDescriptor();

    EXPECT_EQ(dataDescriptor.getName(), "value");

    EXPECT_EQ(dataDescriptor.getSampleType(), SampleType::Float64);
    EXPECT_EQ(dataDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(dataDescriptor.getValueRange(), Range(-15.0, 15.0)); // default set by openDAQ client

    EXPECT_EQ(dataDescriptor.getDimensions().getCount(), 0u);
    EXPECT_EQ(dataDescriptor.getMetadata().getCount(), 0u);
    EXPECT_FALSE(dataDescriptor.getUnit().assigned());

    EXPECT_EQ(domainDescriptor.getName(), "time");
    EXPECT_EQ(domainDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(domainDescriptor.getUnit().getSymbol(), "s");
    EXPECT_EQ(domainDescriptor.getUnit().getQuantity(), "time");
    EXPECT_NE(domainDescriptor.getOrigin(), "");
    EXPECT_NE(domainDescriptor.getTickResolution().getNumerator(), 0);
    EXPECT_NE(domainDescriptor.getTickResolution().getDenominator(), 0);
}

TEST_F(WebsocketSiggenTest, DISABLED_RenderSignals)
{
    auto client = CreateClientInstance();

    const auto rendererFb = client.addFunctionBlock("ref_fb_module_renderer");

    rendererFb.getInputPorts()[0].connect(client.getSignalsRecursive()[0]);
    rendererFb.getInputPorts()[1].connect(client.getSignalsRecursive()[1]);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
