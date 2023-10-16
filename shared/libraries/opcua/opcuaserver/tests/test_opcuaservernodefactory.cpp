#include <gtest/gtest.h>
#include <opcuaserver/opcuaserver.h>
#include <opcuaserver/opcuaservernodefactory.h>
#include "common_test_functions.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaServerNodeFactoryTest = testing::Test;

TEST_F(OpcUaServerNodeFactoryTest, CreateServerNodeFactory)
{
    OpcUaServer server = createServer();
    server.prepare();

    ASSERT_NO_THROW(OpcUaServerNodeFactory nodeFactory(server));
}

TEST_F(OpcUaServerNodeFactoryTest, CreateObject)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaServerNodeFactory nodeFactory(server);

    std::unique_ptr<OpcUaServerNode> newNode;
    ASSERT_NO_THROW(newNode = nodeFactory.createServerNode(OpcUaNodeId(UA_NS0ID_SERVER), OpcUaNodeClass::Object));

    ASSERT_NE(dynamic_cast<OpcUaServerObjectNode*>(newNode.get()), nullptr);
}

TEST_F(OpcUaServerNodeFactoryTest, CreateVariable)
{
    OpcUaServer server = createServer();
    server.prepare();

    OpcUaServerNodeFactory nodeFactory(server);

    std::unique_ptr<OpcUaServerNode> newNode;
    ASSERT_NO_THROW(newNode = nodeFactory.createServerNode(OpcUaNodeId(UA_NS0ID_SERVER_SERVERCAPABILITIES), OpcUaNodeClass::Variable));

    ASSERT_NE(dynamic_cast<OpcUaServerVariableNode*>(newNode.get()), nullptr);
}

END_NAMESPACE_OPENDAQ_OPCUA
