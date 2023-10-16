#include <gtest/gtest.h>
#include <opcuatms_server/tms_server.h>
#include <open62541/di_nodeids.h>
#include <open62541/tmsdevice_nodeids.h>
#include <opcuaclient/reference_utils.h>
#include <tms_object_test.h>
#include "test_helpers.h"

using TmsServerTest = testing::Test;

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua;
using namespace test_helpers;


TEST_F(TmsServerTest, Create)
{
    auto daqInstance = SetupInstance();
    TmsServer server(daqInstance);
}

TEST_F(TmsServerTest, StartStop)
{
    auto daqInstance = SetupInstance();
    TmsServer server(daqInstance);
    server.start();
    server.stop();
}

TEST_F(TmsServerTest, Connect)
{
    auto daqInstance = SetupInstance();
    TmsServer server(daqInstance);
    server.start();

    auto client = TmsObjectTest::CreateAndConnectTestClient();
    ASSERT_TRUE(client->isConnected());
}

TEST_F(TmsServerTest, DeviceTopology)
{
    auto daqInstance = SetupInstance();
    TmsServer server(daqInstance);
    server.start();

    auto client = TmsObjectTest::CreateAndConnectTestClient();

    auto firstLvlDevices = BrowseSubDevices(client, OpcUaNodeId(NAMESPACE_DI, UA_DIID_DEVICESET));
    ASSERT_EQ(firstLvlDevices.size(), 1u);

    auto secondLvlDevices = BrowseSubDevices(client, firstLvlDevices[0]);
    ASSERT_EQ(secondLvlDevices.size(), 2u);
}

TEST_F(TmsServerTest, Channels)
{
    auto daqInstance = SetupInstance();
    TmsServer server(daqInstance);
    server.start();

    auto client = TmsObjectTest::CreateAndConnectTestClient();
    auto mockPhysicalDevice = GetMockPhysicalDevice(client);

    auto inputsOutputsFolder = BrowseForChild(client, mockPhysicalDevice, "InputsOutputs");
    ASSERT_FALSE(inputsOutputsFolder.isNull());

    auto channels = BrowseChannels(client, inputsOutputsFolder);
    ASSERT_EQ(channels.size(), 1u);

    auto signalsNode = BrowseForChild(client, channels[0], "OutputSignals");
    ASSERT_FALSE(signalsNode.isNull());

    auto signals = BrowseSignals(client, signalsNode);
    ASSERT_EQ(signals.size(), 10u);

    auto byteStepSignal = BrowseForChild(client, signalsNode, "ByteStep");
    auto timeSignal = BrowseForChild(client, signalsNode, "Time");

    ReferenceUtils referenceUtils(client);
    referenceUtils.updateReferences(byteStepSignal);

    auto references = referenceUtils.getReferencedNodes(byteStepSignal, OpcUaNodeId(NAMESPACE_TMSBSP, UA_TMSBSPID_HASDOMAINSIGNAL), true);
    ASSERT_EQ(references.size(), 1u);
    ASSERT_EQ(*references.begin(), timeSignal);
}
