#include <gtest/gtest.h>
#include <opcuatms_server/tms_server.h>
#include <open62541/di_nodeids.h>
#include <open62541/daqdevice_nodeids.h>
#include <tms_object_test.h>
#include "test_helpers.h"
#include <opcuaclient/cached_reference_browser.h>

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
    auto referenceBrowser = CachedReferenceBrowser(client);
    auto mockPhysicalDevice = GetMockPhysicalDevice(client);

    auto inputsOutputsFolder = BrowseForChild(client, mockPhysicalDevice, "IO");
    ASSERT_FALSE(inputsOutputsFolder.isNull());

    auto channels = BrowseChannels(client, inputsOutputsFolder);
    ASSERT_EQ(channels.size(), 1u);

    auto signalsNode = BrowseForChild(client, channels[0], "Sig");
    ASSERT_FALSE(signalsNode.isNull());

    auto signals = BrowseSignals(client, signalsNode);
    ASSERT_EQ(signals.size(), 10u);

    auto byteStepSignal = BrowseForChild(client, signalsNode, "ByteStep");
    auto timeSignal = BrowseForChild(client, signalsNode, "Time");

    auto filter = BrowseFilter();
    filter.referenceTypeId = OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASDOMAINSIGNAL);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;

    auto references = referenceBrowser.browseFiltered(byteStepSignal, filter);
    ASSERT_EQ(references.byNodeId.size(), 1u);
    ASSERT_EQ(references.byNodeId.begin().key(), timeSignal);
}
