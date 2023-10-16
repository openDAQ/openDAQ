#include <gtest/gtest.h>
#include <opcuaserver/opcuaserver.h>
#include <opcuaserver/opcuaservernode.h>
#include "common_test_functions.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using namespace utils;

using OpcUaServerNodeTest = testing::Test;

TEST_F(OpcUaServerNodeTest, CreateServerNode)
{
    OpcUaServer server = createServer();
    server.prepare();

    ASSERT_NO_THROW(OpcUaServerNode(server, OpcUaNodeId(UA_NS0ID_SERVER)));
}

TEST_F(OpcUaServerNodeTest, AddObject)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaServerNode serverNode(server, OpcUaNodeId(UA_NS0ID_SERVER));
    ASSERT_NO_THROW(serverNode.addObject(OpcUaNodeId(1, "TestNode"), "TestBrowseName"));
}

TEST_F(OpcUaServerNodeTest, GetNodeClass)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaServerNode serverNode(server, OpcUaNodeId(UA_NS0ID_SERVER));
    ASSERT_EQ(serverNode.getNodeClass(), OpcUaNodeClass::Object);
}

TEST_F(OpcUaServerNodeTest, GetBrowseName)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaServerNode serverNode(server, OpcUaNodeId(UA_NS0ID_SERVER));
    auto browseName = serverNode.getBrowseName();
    ASSERT_EQ(ToStdString(browseName->name), "Server");
    ASSERT_EQ(browseName->namespaceIndex, 0);
}

TEST_F(OpcUaServerNodeTest, GetDisplayName)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaServerNode serverNode(server, OpcUaNodeId(UA_NS0ID_SERVER));
    auto displayName = serverNode.getDisplayName();

    ASSERT_EQ(ToStdString(displayName->text), "Server");
}

TEST_F(OpcUaServerNodeTest, SetDisplayName)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaServerNode serverNode(server, OpcUaNodeId(UA_NS0ID_SERVER));
    
    ASSERT_NO_THROW(serverNode.setDisplayName("test"));

    auto displayName = serverNode.getDisplayName();
    ASSERT_EQ(ToStdString(displayName->text), "test");
}

TEST_F(OpcUaServerNodeTest, AddVariable)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaServerNode serverNode(server, OpcUaNodeId(UA_NS0ID_SERVER));
    ASSERT_NO_THROW(serverNode.addVariable(OpcUaNodeId(1, "TestNode"), "TestBrowseName"));
}

TEST_F(OpcUaServerNodeTest, ReadValue)
{
    OpcUaServer server = createServer();
    server.prepare();

    AddVariableNodeParams params(OpcUaNodeId(1, "TestNode"));
    params.setBrowseName("TestBrowseName");
    params.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_INT64].typeId));
    int64_t value = 5;
    UA_Variant_setScalarCopy(&params.attr->value, &value, &UA_TYPES[UA_TYPES_INT64]);

    auto variableNode = server.getObjectsNode().addVariable(params);
    ASSERT_EQ(variableNode.read<int64_t>(), 5);
    ASSERT_THROW(variableNode.read<UA_String>(), std::runtime_error);
}

TEST_F(OpcUaServerNodeTest, SetValue)
{
    OpcUaServer server = createServer();
    server.prepare();

    AddVariableNodeParams params(OpcUaNodeId(1, "TestNode"));
    params.setBrowseName("TestBrowseName");
    params.setDataType(OpcUaNodeId(UA_TYPES[UA_TYPES_INT64].typeId));

    auto variableNode = server.getObjectsNode().addVariable(params);
    ASSERT_NO_THROW(variableNode.write<int64_t>(5));
    ASSERT_EQ(variableNode.read<int64_t>(), 5);
}

TEST_F(OpcUaServerNodeTest, BrowseChildNodes)
{
    OpcUaServer server = createServer();
    server.prepare();

    auto parentNode = server.getObjectsNode().addObject(OpcUaNodeId(0, "ParentNode"), "ParentNode");
    auto childNode = parentNode.addObject(OpcUaNodeId(0, "ChildNode"), "ChildNode");

    auto result = parentNode.browseChildNodes();
    ASSERT_EQ(result.size(), 1u);
    const auto& firstNode = result[0];
    ASSERT_NE(dynamic_cast<OpcUaServerObjectNode*>(firstNode.get()), nullptr);
    ASSERT_EQ(firstNode->getNodeId(), OpcUaNodeId(0, "ChildNode"));
    ASSERT_EQ(ToStdString(firstNode->getBrowseName()->name), "ChildNode");
}

END_NAMESPACE_OPENDAQ_OPCUA
