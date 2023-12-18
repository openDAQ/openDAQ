#include "tms_object_test.h"
#include "opcuaclient/opcuaclient.h"
#include <open62541/types_daqdevice_generated.h>
#include <open62541/types_daqbsp_generated.h>
#include <open62541/types_di_generated.h>

using namespace daq::opcua;

TmsObjectTest::TmsObjectTest()
{
}

void TmsObjectTest::SetUp()
{
    testing::Test::SetUp();

    server = CreateAndStartTestServer();
    client = CreateAndConnectTestClient();
}

void TmsObjectTest::TearDown()
{
    client.reset();
    server.reset();
    testing::Test::TearDown();
}

OpcUaServerPtr TmsObjectTest::getServer()
{
    return server;
}

OpcUaClientPtr TmsObjectTest::getClient()
{
    return client;
}

void TmsObjectTest::writeChildNode(const OpcUaNodeId& parent, const std::string& browseName, const OpcUaVariant& variant)
{
    getClient()->writeValue(getChildNodeId(parent, browseName), variant);
}

OpcUaVariant TmsObjectTest::readChildNode(const OpcUaNodeId& parent, const std::string& browseName)
{
    return getClient()->readValue(getChildNodeId(parent, browseName));
}

OpcUaNodeId TmsObjectTest::getChildNodeId(const OpcUaNodeId& parent, const std::string& browseName)
{
    OpcUaObject<UA_BrowseRequest> br;
    br->requestedMaxReferencesPerNode = 0;
    br->nodesToBrowse = UA_BrowseDescription_new();
    br->nodesToBrowseSize = 1;
    br->nodesToBrowse[0].nodeId = parent.copyAndGetDetachedValue();
    br->nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;

    OpcUaObject<UA_BrowseResponse> result = UA_Client_Service_browse(client->getUaClient(), *br);

    if (result->resultsSize == 0)
        return OpcUaNodeId(UA_NODEID_NULL);

    auto references = result->results[0].references;
    auto referenceCount = result->results[0].referencesSize;

    for (size_t i = 0; i < referenceCount; i++)
    {
        auto reference = references[i];
        std::string refBrowseName = utils::ToStdString(reference.browseName.name);
        if (refBrowseName == browseName)
            return OpcUaNodeId(reference.nodeId.nodeId);
    }

    return OpcUaNodeId(UA_NODEID_NULL);
}

void TmsObjectTest::waitForInput()
{
    std::cout << "Type q to quit..." << std::endl;
    std::string input;
    std::cin >> input;
}

daq::opcua::OpcUaServerPtr TmsObjectTest::CreateAndStartTestServer()
{
    auto server = std::make_shared<daq::opcua::OpcUaServer>();
    server->setPort(4840);
    server->start();
    return server;
}

daq::opcua::OpcUaClientPtr TmsObjectTest::CreateAndConnectTestClient()
{
    OpcUaEndpoint endpoint("Test", "opc.tcp://127.0.0.1:4840");
    endpoint.registerCustomTypes(UA_TYPES_DI_COUNT, UA_TYPES_DI);
    endpoint.registerCustomTypes(UA_TYPES_DAQBT_COUNT, UA_TYPES_DAQBT);
    endpoint.registerCustomTypes(UA_TYPES_DAQBSP_COUNT, UA_TYPES_DAQBSP);
    endpoint.registerCustomTypes(UA_TYPES_DAQDEVICE_COUNT, UA_TYPES_DAQDEVICE);

    auto client = std::make_shared<daq::opcua::OpcUaClient>(endpoint);
    client->connect();
    client->runIterate();
    return client;
}
