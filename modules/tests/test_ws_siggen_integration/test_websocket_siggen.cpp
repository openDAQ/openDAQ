#include <testutils/testutils.h>
#include <opendaq/opendaq.h>
#include "testutils/memcheck_listener.h"
#include <thread>

using SiggenTest = testing::TestWithParam<std::string>;

using namespace daq;

static InstancePtr CreateClientInstance(std::string connectionString)
{
    auto instance = Instance();
    auto refDevice = instance.addDevice(connectionString);
    return instance;
}

// signals configuration is set by "siggen_config.json"

TEST_P(SiggenTest, ConnectAndDisconnect)
{
    auto client = CreateClientInstance(GetParam());
}

TEST_P(SiggenTest, GetRemoteDeviceObjects)
{
    auto client = CreateClientInstance(GetParam());

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto signals = client.getSignals(SearchParams(false, true));
    ASSERT_EQ(signals.getCount(), 2u);
}

TEST_P(SiggenTest, SyncSignalDescriptors)
{
    auto client = CreateClientInstance(GetParam());

    auto signal  = client.getSignals(SearchParams(false, true))[0];

    EXPECT_EQ(signal.getLocalId(), "Signal1_Sync");

    DataDescriptorPtr dataDescriptor = signal.getDescriptor();
    DataDescriptorPtr domainDescriptor = signal.getDomainSignal().getDescriptor();

    EXPECT_EQ(signal.getName(), "value");

    EXPECT_EQ(dataDescriptor.getSampleType(), SampleType::Float64);
    EXPECT_EQ(dataDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(dataDescriptor.getValueRange(), Range(-15.0, 15.0)); // default set by openDAQ client

    EXPECT_EQ(dataDescriptor.getDimensions().getCount(), 0u);
    EXPECT_EQ(dataDescriptor.getMetadata().getCount(), 0u);
    EXPECT_FALSE(dataDescriptor.getUnit().assigned());

    EXPECT_EQ(domainDescriptor.getRule().getType(), DataRuleType::Linear);
    EXPECT_EQ(domainDescriptor.getUnit().getSymbol(), "s");
    EXPECT_EQ(domainDescriptor.getUnit().getQuantity(), "time");
    EXPECT_NE(domainDescriptor.getOrigin(), "");
    EXPECT_NE(domainDescriptor.getTickResolution().getNumerator(), 0);
    EXPECT_NE(domainDescriptor.getTickResolution().getDenominator(), 0);
}

TEST_P(SiggenTest, AsyncSignalDescriptors)
{
    auto client = CreateClientInstance(GetParam());

    auto signal  = client.getSignals(SearchParams(false, true))[1];

    EXPECT_EQ(signal.getLocalId(), "Signal2_Async");

    DataDescriptorPtr dataDescriptor = signal.getDescriptor();
    DataDescriptorPtr domainDescriptor = signal.getDomainSignal().getDescriptor();

    EXPECT_EQ(signal.getName(), "value");

    EXPECT_EQ(dataDescriptor.getSampleType(), SampleType::Float64);
    EXPECT_EQ(dataDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(dataDescriptor.getValueRange(), Range(-15.0, 15.0)); // default set by openDAQ client

    EXPECT_EQ(dataDescriptor.getDimensions().getCount(), 0u);
    EXPECT_EQ(dataDescriptor.getMetadata().getCount(), 0u);
    EXPECT_FALSE(dataDescriptor.getUnit().assigned());

    EXPECT_EQ(domainDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(domainDescriptor.getUnit().getSymbol(), "s");
    EXPECT_EQ(domainDescriptor.getUnit().getQuantity(), "time");
    EXPECT_NE(domainDescriptor.getOrigin(), "");
    EXPECT_NE(domainDescriptor.getTickResolution().getNumerator(), 0);
    EXPECT_NE(domainDescriptor.getTickResolution().getDenominator(), 0);
}

TEST_P(SiggenTest, DISABLED_RenderSignals)
{
    auto client = CreateClientInstance(GetParam());

    const auto rendererFb = client.addFunctionBlock("ref_fb_module_renderer");

    rendererFb.getInputPorts()[0].connect(client.getSignals(SearchParams(false, true))[0]);
    rendererFb.getInputPorts()[1].connect(client.getSignals(SearchParams(false, true))[1]);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

INSTANTIATE_TEST_SUITE_P(
    SiggenTestGroup,
    SiggenTest,
    testing::Values(
        "daq.tcp://127.0.0.1:7411/",
        "daq.ws://127.0.0.1:7413/"
        )
    );
