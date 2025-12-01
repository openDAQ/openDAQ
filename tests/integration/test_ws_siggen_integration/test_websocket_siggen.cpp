#include <testutils/testutils.h>
#include <opendaq/opendaq.h>
#include "testutils/memcheck_listener.h"
#include <chrono>
#include <thread>

using SiggenTest = testing::TestWithParam<std::string>;

using namespace daq;

static InstancePtr CreateClientInstance(std::string connectionString)
{
    auto instance = Instance();
    auto refDevice = instance.addDevice(connectionString);
    // Wait for signals to become available
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return instance;
}

// signals configuration is set by "siggen_config.json"

TEST_P(SiggenTest, ConnectAndDisconnect)
{
    auto client = CreateClientInstance(GetParam());
}

// FIXME - time signals remain unknown

TEST_P(SiggenTest, DISABLED_GetRemoteDeviceObjects)
{
    auto client = CreateClientInstance(GetParam());

    ASSERT_EQ(client.getDevices().getCount(), 1u);
    auto signals = client.getSignalsRecursive();
    ASSERT_EQ(signals.getCount(), 4u);
}

TEST_P(SiggenTest, DISABLED_SyncSignalDescriptors)
{
    auto client = CreateClientInstance(GetParam());

    MirroredSignalConfigPtr signal;
    for (const auto& s : client.getSignalsRecursive())
        if (s.getLocalId() == "Signal1_Sync")
            signal = s;

    ASSERT_TRUE(signal.assigned());

    DataDescriptorPtr dataDescriptor = signal.getDescriptor();
    DataDescriptorPtr domainDescriptor = signal.getDomainSignal().getDescriptor();

    EXPECT_EQ(signal.getName(), "value");

    EXPECT_EQ(dataDescriptor.getSampleType(), SampleType::Float64);
    EXPECT_EQ(dataDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(dataDescriptor.getValueRange(), Range(-15, 15));

    EXPECT_FALSE(dataDescriptor.getPostScaling().assigned());

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

TEST_P(SiggenTest, DISABLED_SyncPostScalingSignalDescriptors)
{
    auto client = CreateClientInstance(GetParam());

    MirroredSignalConfigPtr signal;
    for (const auto& s : client.getSignalsRecursive())
        if (s.getLocalId() == "Signal2_Sync_PostScaling")
            signal = s;

    ASSERT_TRUE(signal.assigned());

    DataDescriptorPtr dataDescriptor = signal.getDescriptor();
    DataDescriptorPtr domainDescriptor = signal.getDomainSignal().getDescriptor();

    EXPECT_EQ(signal.getName(), "value");

    EXPECT_EQ(dataDescriptor.getSampleType(), SampleType::Float64);
    EXPECT_EQ(dataDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(dataDescriptor.getValueRange(), Range(-15, 15));

    ASSERT_TRUE(dataDescriptor.getPostScaling().assigned());
    EXPECT_EQ(dataDescriptor.getPostScaling().getParameters().get("scale"), 2);
    EXPECT_EQ(dataDescriptor.getPostScaling().getParameters().get("offset"), 5);

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

TEST_P(SiggenTest, DISABLED_AsyncSignalDescriptors)
{
    auto client = CreateClientInstance(GetParam());

    MirroredSignalConfigPtr signal;
    for (const auto& s : client.getSignalsRecursive())
        if (s.getLocalId() == "Signal2_Async")
            signal = s;

    ASSERT_TRUE(signal.assigned());

    DataDescriptorPtr dataDescriptor = signal.getDescriptor();
    DataDescriptorPtr domainDescriptor = signal.getDomainSignal().getDescriptor();

    EXPECT_EQ(signal.getName(), "value");

    EXPECT_EQ(dataDescriptor.getSampleType(), SampleType::Float64);
    EXPECT_EQ(dataDescriptor.getRule().getType(), DataRuleType::Explicit);
    EXPECT_EQ(dataDescriptor.getValueRange(), Range(-15, 15));

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

    const auto rendererFb = client.addFunctionBlock("RefFBModuleRenderer");

    MirroredSignalConfigPtr signal1;
    for (const auto& s : client.getSignalsRecursive())
        if (s.getLocalId() == "Signal1_Sync")
            signal1 = s;
    ASSERT_TRUE(signal1.assigned());

    MirroredSignalConfigPtr signal2;
    for (const auto& s : client.getSignalsRecursive())
        if (s.getLocalId() == "Signal2_Sync_PostScaling")
            signal2 = s;
    ASSERT_TRUE(signal2.assigned());

    rendererFb.getInputPorts()[0].connect(signal1);
    rendererFb.getInputPorts()[1].connect(signal2);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

INSTANTIATE_TEST_SUITE_P(
    SiggenTestGroup,
    SiggenTest,
    testing::Values(
        "daq.lt://127.0.0.1:7413/"
        )
    );
