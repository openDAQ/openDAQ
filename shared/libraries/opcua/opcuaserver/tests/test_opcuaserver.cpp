#include <opcuaclient/taskprocessor/opcuataskprocessor.h>
#include <opcuaserver/opcuaserver.h>
#include <opcuashared/node/opcuaobjecttype.h>
#include <opcuashared/node/opcuavariabletype.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include <fstream>
#include <future>
#include "common_test_functions.h"
#include <testutils/testutils.h>
#include <opcuaclient/monitored_item_create_request.h>
#include <opcuaclient/event_filter.h>

#include <open62541/di_nodeids.h>
#ifdef NAMESPACE_TMSBT
    #include <open62541/tmsbt_nodeids.h>
#endif
#ifdef NAMESPACE_TMSBSP
    #include <open62541/tmsbsp_nodeids.h>
#endif
#ifdef NAMESPACE_TMSDEVICE
    #include <open62541/tmsdevice_nodeids.h>
#endif

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using namespace utils;
using namespace std::chrono_literals;

using OpcUaServerTest = testing::Test;

#define ASSERT_EQ_STATUS(status, expectedStatus) ASSERT_EQ(status, (UA_StatusCode) expectedStatus)
#define ASSERT_STATUSCODE_GOOD(status) ASSERT_EQ_STATUS(status, UA_STATUSCODE_GOOD)

TEST_F(OpcUaServerTest, StartStopTest)
{
    OpcUaServer server = createServer();
    ASSERT_FALSE(server.isPrepared());
    ASSERT_FALSE(server.getStarted());

    server.prepare();
    ASSERT_TRUE(server.isPrepared());
    ASSERT_FALSE(server.getStarted());

    server.start();
    ASSERT_TRUE(server.isPrepared());
    ASSERT_TRUE(server.getStarted());

    server.stop();
    ASSERT_FALSE(server.isPrepared());
    ASSERT_FALSE(server.getStarted());
}

TEST_F(OpcUaServerTest, StopWithoutStartTest)
{
    OpcUaServer server = createServer();

    server.prepare();

    server.stop();
    ASSERT_FALSE(server.isPrepared());
    ASSERT_FALSE(server.getStarted());
}

TEST_F(OpcUaServerTest, NodeExists)
{
    OpcUaServer server = createServer();
    server.prepare();

    ASSERT_TRUE(server.nodeExists(OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER)));

    ASSERT_FALSE(server.nodeExists(OpcUaNodeId(1, "unknown")));
}

TEST_F(OpcUaServerTest, ReferenceExists)
{
    OpcUaServer server = createServer();
    server.prepare();

    ASSERT_TRUE(server.referenceExists(
        OpcUaNodeId(UA_NS0ID_ROOTFOLDER), OpcUaNodeId(UA_NS0ID_ORGANIZES), OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER), true));
    ASSERT_TRUE(server.referenceExists(
        OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER), OpcUaNodeId(UA_NS0ID_ORGANIZES), OpcUaNodeId(UA_NS0ID_ROOTFOLDER), false));

    ASSERT_FALSE(server.referenceExists(
        OpcUaNodeId(UA_NS0ID_ROOTFOLDER), OpcUaNodeId(UA_NS0ID_ORGANIZES), OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER), false));
    ASSERT_FALSE(server.referenceExists(
        OpcUaNodeId(UA_NS0ID_ROOTFOLDER), OpcUaNodeId(UA_NS0ID_ORGANIZES), OpcUaNodeId(UA_NS0ID_DATATYPESFOLDER), true));
}

TEST_F_OPTIONAL(OpcUaServerTest, ClientConnectTest)
{
    OpcUaServer server = createServer();
    server.start();

    UA_Client* client = CreateClient();
    ASSERT_EQ_STATUS(UA_Client_connect(client, SERVER_URL), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);

    server.stop();
}

TEST_F_OPTIONAL(OpcUaServerTest, ClientConnectTestStopBeforeClientDisconnect)
{
    OpcUaServer server = createServer();
    server.start();

    UA_Client* client = CreateClient();
    ASSERT_EQ_STATUS(UA_Client_connect(client, SERVER_URL), UA_STATUSCODE_GOOD);

    server.stop();

    UA_Client_delete(client);
}

TEST_F_OPTIONAL(OpcUaServerTest, ClientConnectTestSessionContext)
{
    OpcUaServer server = createServer();
    server.createSessionContextCallback = [](const OpcUaNodeId& sessionId) { return new int; };
    server.deleteSessionContextCallback = [](void* pointer) { delete (int*) pointer; };
    server.start();

    UA_Client* client = CreateClient();
    ASSERT_EQ_STATUS(UA_Client_connect(client, SERVER_URL), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);

    server.stop();
}

TEST_F_OPTIONAL(OpcUaServerTest, ClientConnectTestSessionContextStopBeforeClientDisconnect)
{
    OpcUaServer server = createServer();
    server.createSessionContextCallback = [](const OpcUaNodeId& sessionId) { return new int; };
    server.deleteSessionContextCallback = [](void* pointer) { delete (int*) pointer; };
    server.start();

    UA_Client* client = CreateClient();
    ASSERT_EQ_STATUS(UA_Client_connect(client, SERVER_URL), UA_STATUSCODE_GOOD);

    server.stop();

    UA_Client_delete(client);
}

TEST_F(OpcUaServerTest, ReadBrowseName)
{
    OpcUaServer server = createServer();
    server.prepare();

    auto qualifiedName = server.readBrowseName(OpcUaNodeId(UA_NS0ID_SERVER));
    ASSERT_EQ(qualifiedName->namespaceIndex, 0);
    ASSERT_EQ(ToStdString(qualifiedName->name), "Server");

    ASSERT_THROW(server.readBrowseName(OpcUaNodeId(0, "Unknown")), OpcUaException);
}

TEST_F(OpcUaServerTest, ReadDisplayName)
{
    OpcUaServer server = createServer();
    server.prepare();

    ASSERT_EQ(ToStdString(server.readDisplayName(OpcUaNodeId(UA_NS0ID_SERVER))->text), "Server");

    ASSERT_THROW(server.readDisplayName(OpcUaNodeId(0, "Unknown")), OpcUaException);
}

TEST_F(OpcUaServerTest, AddFolderTest)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId nodeId(1, "FolderId");

    AddObjectNodeParams params(nodeId, OpcUaNodeId(UA_NS0ID_SERVER));
    params.setBrowseName("Folder");

    OpcUaNodeId outNodeId = server.addObjectNode(params);
    ASSERT_EQ(OpcUaNodeId(1, "FolderId"), outNodeId);

    ASSERT_TRUE(server.nodeExists(nodeId));

    OpcUaObject<UA_QualifiedName> qualifiedName = server.readBrowseName(nodeId);
    ASSERT_EQ(qualifiedName->namespaceIndex, 0);
    ASSERT_EQ(ToStdString(qualifiedName->name), "Folder");

    ASSERT_EQ(ToStdString(server.readDisplayName(nodeId)->text), "Folder");

    ASSERT_THROW(server.addObjectNode(params), OpcUaException);  // Duplicate
}

TEST_F(OpcUaServerTest, AddVariableTest)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId nodeId(1, "VariableId");

    AddVariableNodeParams params(nodeId, OpcUaNodeId(UA_NS0ID_SERVER));
    params.setBrowseName("Variable");

    OpcUaNodeId outNodeId = server.addVariableNode(params);
    ASSERT_EQ(OpcUaNodeId(1, "VariableId"), outNodeId);

    ASSERT_TRUE(server.nodeExists(nodeId));

    OpcUaObject<UA_QualifiedName> qualifiedName = server.readBrowseName(nodeId);
    ASSERT_EQ(qualifiedName->namespaceIndex, 0);
    ASSERT_EQ(ToStdString(qualifiedName->name), "Variable");

    ASSERT_EQ(ToStdString(server.readDisplayName(nodeId)->text), "Variable");

    ASSERT_THROW(server.addVariableNode(params), OpcUaException);  // Duplicate
}

TEST_F(OpcUaServerTest, AddMethodTest)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId nodeId(1, "MethodId");

    AddMethodNodeParams params(nodeId, OpcUaNodeId(UA_NS0ID_SERVER));
    params.setBrowseName("Method");

    OpcUaNodeId outNodeId = server.addMethodNode(params);
    ASSERT_EQ(OpcUaNodeId(1, "MethodId"), outNodeId);

    ASSERT_TRUE(server.nodeExists(nodeId));

    OpcUaObject<UA_QualifiedName> qualifiedName = server.readBrowseName(nodeId);
    ASSERT_EQ(qualifiedName->namespaceIndex, 0);
    ASSERT_EQ(ToStdString(qualifiedName->name), "Method");

    ASSERT_EQ(ToStdString(server.readDisplayName(nodeId)->text), "Method");

    ASSERT_THROW(server.addMethodNode(params), OpcUaException);  // Duplicate
}

TEST_F(OpcUaServerTest, AddObjectType)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId nodeId(1, "ObjectTypeId");

    AddObjectTypeNodeParams params(nodeId, OpcUaNodeId(UA_NS0ID_BASEOBJECTTYPE));
    params.setBrowseName("ObjectType");

    OpcUaNodeId outNodeId = server.addObjectTypeNode(params);
    ASSERT_EQ(OpcUaNodeId(1, "ObjectTypeId"), outNodeId);

    ASSERT_TRUE(server.nodeExists(nodeId));

    OpcUaObject<UA_QualifiedName> qualifiedName = server.readBrowseName(nodeId);
    ASSERT_EQ(qualifiedName->namespaceIndex, 0);
    ASSERT_EQ(ToStdString(qualifiedName->name), "ObjectType");

    ASSERT_EQ(ToStdString(server.readDisplayName(nodeId)->text), "ObjectType");

    ASSERT_THROW(server.addObjectTypeNode(params), OpcUaException);  // Duplicate
}

TEST_F(OpcUaServerTest, AddVariableType)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId nodeId(1, "VariableTypeId");

    AddVariableTypeNodeParams params(nodeId, OpcUaNodeId(UA_NS0ID_BASEVARIABLETYPE));
    params.setBrowseName("VariableType");

    OpcUaNodeId outNodeId = server.addVariableTypeNode(params);
    ASSERT_EQ(OpcUaNodeId(1, "VariableTypeId"), outNodeId);

    ASSERT_TRUE(server.nodeExists(nodeId));

    OpcUaObject<UA_QualifiedName> qualifiedName = server.readBrowseName(nodeId);
    ASSERT_EQ(qualifiedName->namespaceIndex, 0);
    ASSERT_EQ(ToStdString(qualifiedName->name), "VariableType");

    ASSERT_EQ(ToStdString(server.readDisplayName(nodeId)->text), "VariableType");

    ASSERT_THROW(server.addVariableTypeNode(params), OpcUaException);  // Duplicate
}

TEST_F(OpcUaServerTest, AddDeleteReference)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId sourceId(0, UA_NS0ID_OBJECTSFOLDER);
    OpcUaNodeId refTypeId(0, UA_NS0ID_HASCOMPONENT);
    OpcUaNodeId targetId(0, UA_NS0ID_SERVER_VENDORSERVERINFO);

    ASSERT_FALSE(server.referenceExists(sourceId, refTypeId, targetId, true));

    server.addReference(sourceId, refTypeId, targetId, true);

    ASSERT_TRUE(server.referenceExists(sourceId, refTypeId, targetId, true));

    server.deleteReference(sourceId, refTypeId, targetId, true);

    ASSERT_FALSE(server.referenceExists(sourceId, refTypeId, targetId, true));
}

TEST_F(OpcUaServerTest, DINodeSetTest)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId uaNodeId(2, UA_DIID_TRANSFERRESULTDATADATATYPE);

    OpcUaObject<UA_QualifiedName> qualifiedName;
    ASSERT_NO_THROW(qualifiedName = server.readBrowseName(uaNodeId));
    ASSERT_EQ(ToStdString(qualifiedName->name), "TransferResultDataDataType");
}
#ifdef NAMESPACE_TMSBT
TEST_F(OpcUaServerTest, TmsBtNodeSetTest)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId uaNodeId(3, UA_TMSBTID_DAQBASEOBJECTTYPE);

    OpcUaObject<UA_QualifiedName> qualifiedName;
    ASSERT_NO_THROW(qualifiedName = server.readBrowseName(uaNodeId));
    ASSERT_EQ(ToStdString(qualifiedName->name), "DaqBaseObjectType");
}
#endif

#ifdef NAMESPACE_TMSBSP
TEST_F(OpcUaServerTest, TmsBspNodeSetTest)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId uaNodeId(4, UA_TMSBSPID_SIGNALTYPE);

    OpcUaObject<UA_QualifiedName> qualifiedName;
    ASSERT_NO_THROW(qualifiedName = server.readBrowseName(uaNodeId));
    ASSERT_EQ(ToStdString(qualifiedName->name), "SignalType");
}
#endif

#ifdef NAMESPACE_TMSDEVICE
TEST_F(OpcUaServerTest, TmsDeviceNodeSetTest)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaNodeId uaNodeId(5, UA_TMSDEVICEID_DAQDEVICETYPE);

    OpcUaObject<UA_QualifiedName> qualifiedName;
    ASSERT_NO_THROW(qualifiedName = server.readBrowseName(uaNodeId));
    ASSERT_EQ(ToStdString(qualifiedName->name), "DaqDeviceType");
}
#endif

TEST_F(OpcUaServerTest, GetServerNodes)
{
    OpcUaServer server = createServer();
    server.prepare();

    ASSERT_NO_THROW(server.getNode(OpcUaNodeId(UA_NS0ID_SERVER)));
    ASSERT_NO_THROW(server.getRootNode());
    ASSERT_NO_THROW(server.getObjectsNode());
    ASSERT_NO_THROW(server.getTypesNode());
    ASSERT_NO_THROW(server.getViewsNode());
    ASSERT_NO_THROW(server.getObjectTypesNode());
    ASSERT_NO_THROW(server.getVariableTypesNode());
    ASSERT_NO_THROW(server.getDataTypesNode());
    ASSERT_NO_THROW(server.getReferenceTypesNode());
}

const size_t nSelectClauses = 1;

static UA_SimpleAttributeOperand* setupSelectClauses(void)
{
    UA_SimpleAttributeOperand* selectClauses =
        (UA_SimpleAttributeOperand*) UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);

    selectClauses[0] = SimpleAttributeOperand::CreateMessageValue().getDetachedValue();

    return selectClauses;
}

TEST_F(OpcUaServerTest, TriggerEvent)
{
    OpcUaServer server = createServer();
    server.start();

    auto client = CreateClientAndConnect();
    client->runIterate();

    OpcUaObject<UA_CreateSubscriptionRequest> request = UA_CreateSubscriptionRequest_default();

    Subscription* subscription = client->createSubscription(request);

    /* Add a MonitoredItem */
    EventMonitoredItemCreateRequest item(UA_NODEID_NUMERIC(0, 2253));
    UA_EventFilter* filter = UA_EventFilter_new();
    UA_EventFilter_init(filter);
    filter->selectClauses = setupSelectClauses();
    filter->selectClausesSize = nSelectClauses;

    item.setEventFilter(filter);

    std::promise<std::string> promise;
    subscription->monitoredItemsCreateEvent(UA_TIMESTAMPSTORETURN_BOTH,
                                                          *item,
                                                          [&promise](OpcUaClient* client,
                                                                     Subscription* subContext,
                                                                     MonitoredItem* monContext,
                                                                     size_t nEventFields,
                                                                     UA_Variant* eventFields) {
                                                              OpcUaVariant val(eventFields[0]);
                                                              UA_LocalizedText localizedText = val.readScalar<UA_LocalizedText>();
                                                              promise.set_value(ToStdString(localizedText.text));
                                                          });

    EventAttributes eventAttributes;
    eventAttributes.setMessage("", "TestMessage");
    ASSERT_NO_THROW(server.triggerEvent(OpcUaNodeId(UA_NS0ID_BASEEVENTTYPE), OpcUaNodeId(UA_NS0ID_SERVER), eventAttributes));

    auto future = promise.get_future();
    ASSERT_NE(future.wait_for(2s), std::future_status::timeout);
    ASSERT_EQ(future.get(), "TestMessage");
}

END_NAMESPACE_OPENDAQ_OPCUA
