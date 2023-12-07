#include "testutils/testutils.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuaservertesthelper.h"
#include "opcuashared/opcua.h"
#include "opcuashared/opcuacommon.h"
#include "opcuaclient/cached_reference_browser.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using CachedReferenceBrowserTest = BaseClientTest;

TEST_F(CachedReferenceBrowserTest, ObjectsFolder)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    auto nodeId = OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER);

    auto browser = CachedReferenceBrowser(client);
    ASSERT_NO_THROW(browser.browse(nodeId));

    auto references = browser.browse(nodeId);
    ASSERT_GT(references.byNodeId.size(), 0u);

    ASSERT_NO_THROW(browser.invalidate(nodeId));
}

TEST_F(CachedReferenceBrowserTest, ServerStatus)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    auto nodeId = OpcUaNodeId(UA_NS0ID_SERVER_SERVERSTATUS);

    auto browser = CachedReferenceBrowser(client);
    auto serverStatusRefs = browser.browse(OpcUaNodeId(UA_NS0ID_SERVER_SERVERSTATUS));

    ASSERT_EQ(serverStatusRefs.byBrowseName.size(), 7u);
    ASSERT_EQ(serverStatusRefs.byNodeId.size(), serverStatusRefs.byBrowseName.size());

    auto begin = serverStatusRefs.byBrowseName.begin();
    ASSERT_EQ("ServerStatusType", (begin + 0).key());
    ASSERT_EQ("ShutdownReason", (begin + 1).key());
    ASSERT_EQ("SecondsTillShutdown", (begin + 2).key());
    ASSERT_EQ("BuildInfo", (begin + 3).key());
    ASSERT_EQ("State", (begin + 4).key());
    ASSERT_EQ("CurrentTime", (begin + 5).key());
    ASSERT_EQ("StartTime", (begin + 6).key());

    auto buildInfoRefs = browser.browse(OpcUaNodeId(UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO));
    ASSERT_EQ(buildInfoRefs.byBrowseName.size(), 7u);

    begin = buildInfoRefs.byBrowseName.begin();
    ASSERT_EQ("BuildInfoType", (begin + 0).key());
    ASSERT_EQ("BuildDate", (begin + 1).key());
    ASSERT_EQ("BuildNumber", (begin + 2).key());
    ASSERT_EQ("SoftwareVersion", (begin + 3).key());
    ASSERT_EQ("ManufacturerName", (begin + 4).key());
    ASSERT_EQ("ProductUri", (begin + 5).key());
    ASSERT_EQ("ProductName", (begin + 6).key());

    auto buildDateRefs = browser.browse(OpcUaNodeId(UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE));
    ASSERT_EQ(buildDateRefs.byBrowseName.size(), 1u);

    begin = buildDateRefs.byBrowseName.begin();
    ASSERT_EQ("BaseDataVariableType", (begin + 0).key());
}

TEST_F(CachedReferenceBrowserTest, Invalidate)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    auto nodeObjectsFolder = OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER);
    auto browser = CachedReferenceBrowser(client);
    auto references = browser.browse(nodeObjectsFolder);

    ASSERT_GT(references.byNodeId.size(), 0u);

    UA_Double doubleVal = 10.5;
    testHelper.publishVariable(".anotherVar", &doubleVal, &UA_TYPES[UA_TYPES_DOUBLE], nodeObjectsFolder.getPtr());

    auto referencesFromCache = browser.browse(nodeObjectsFolder);
    ASSERT_EQ(references.byNodeId.size(), referencesFromCache.byNodeId.size());

    browser.invalidate(nodeObjectsFolder);

    auto referencesAfterInvalidate = browser.browse(nodeObjectsFolder);
    ASSERT_EQ(references.byNodeId.size() + 1, referencesAfterInvalidate.byNodeId.size());
}

TEST_F(CachedReferenceBrowserTest, IsSubtypeOf)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto shelvedStateMachineType = OpcUaNodeId(UA_NS0ID_SHELVEDSTATEMACHINETYPE);
    const auto finiteStateMachineType = OpcUaNodeId(UA_NS0ID_FINITESTATEMACHINETYPE);
    const auto stateMachineType = OpcUaNodeId(UA_NS0ID_STATEMACHINETYPE);
    const auto serverType = OpcUaNodeId(UA_NS0ID_SERVERTYPE);

    auto browser = CachedReferenceBrowser(client);

    ASSERT_TRUE(browser.isSubtypeOf(shelvedStateMachineType, stateMachineType));
    ASSERT_TRUE(browser.isSubtypeOf(finiteStateMachineType, stateMachineType));
    ASSERT_TRUE(browser.isSubtypeOf(finiteStateMachineType, finiteStateMachineType));
    ASSERT_FALSE(browser.isSubtypeOf(shelvedStateMachineType, serverType));
}

TEST_F(CachedReferenceBrowserTest, HasReference)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto nodeObjectsFolder = OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER);
    auto browser = CachedReferenceBrowser(client);

    ASSERT_TRUE(browser.hasReference(nodeObjectsFolder, ".b"));
    ASSERT_TRUE(browser.hasReference(nodeObjectsFolder, ".ui16"));
    ASSERT_FALSE(browser.hasReference(nodeObjectsFolder, "MissingNode"));
}

TEST_F(CachedReferenceBrowserTest, GetChildNodeId)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    const auto nodeObjectsFolder = OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER);
    auto browser = CachedReferenceBrowser(client);

    ASSERT_EQ(browser.getChildNodeId(nodeObjectsFolder, ".b"), OpcUaNodeId(1, ".b"));
    ASSERT_EQ(browser.getChildNodeId(nodeObjectsFolder, ".ui16"), OpcUaNodeId(1, ".ui16"));
    ASSERT_EQ(browser.getChildNodeId(nodeObjectsFolder, "Server"), OpcUaNodeId(UA_NS0ID_SERVER));
}

TEST_F(CachedReferenceBrowserTest, BrowseFiltered)
{
    auto client = std::make_shared<OpcUaClient>(getServerUrl());
    client->connect();

    auto browser = CachedReferenceBrowser(client);

    const auto nodeId = OpcUaNodeId(UA_NS0ID_SERVER);

    BrowseFilter filter;
    filter.referenceTypeId = OpcUaNodeId(UA_NS0ID_HASPROPERTY);
    filter.direction = UA_BROWSEDIRECTION_FORWARD;

    const auto& children = browser.browseFiltered(nodeId, filter);
    ASSERT_EQ(children.byNodeId.size(), 4u);
}


END_NAMESPACE_OPENDAQ_OPCUA
