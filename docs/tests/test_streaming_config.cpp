#include <gtest/gtest.h>
#include <opendaq/opendaq.h>
#include "docs_test_helpers.h"

using StreamingConfigTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_configure_streaming.adoc
TEST_F(StreamingConfigTest, AddPseudoDevice)
{
    SKIP_TEST_MAC_CI;

    InstancePtr server = docs_test_helpers::setupSimulatorServers();
    InstancePtr instance = Instance();

    DevicePtr device = instance.addDevice("daq.ns://127.0.0.1");
    ASSERT_TRUE(device.assigned());

    ASSERT_EQ(device.getInfo().getName(), "NativeStreamingClientPseudoDevice");
}

END_NAMESPACE_OPENDAQ
