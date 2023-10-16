#include "gtest/gtest.h"
#include "opcuashared/opcuanodeid.h"
#include <open62541/types_generated_handling.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaNodeIdTest = testing::Test;

TEST_F(OpcUaNodeIdTest, CreateNull)
{
    OpcUaNodeId nodeId;
    ASSERT_TRUE(nodeId.isNull());
}

TEST_F(OpcUaNodeIdTest, CreateInt)
{
    OpcUaNodeId nodeId(1, 2);

    ASSERT_EQ(nodeId.getNamespaceIndex(), 1);
    ASSERT_EQ(nodeId.getIdentifierType(), OpcUaIdentifierType::Numeric);
    ASSERT_EQ(nodeId.getIdentifier(), "2");
}

TEST_F(OpcUaNodeIdTest, CreateString)
{
    OpcUaNodeId nodeId(1, "TestIden");

    ASSERT_EQ(nodeId.getNamespaceIndex(), 1);
    ASSERT_EQ(nodeId.getIdentifierType(), OpcUaIdentifierType::String);
    ASSERT_EQ(nodeId.getIdentifier(), "TestIden");
}

TEST_F(OpcUaNodeIdTest, CreateFromOpcUaNodeInt)
{
    UA_NodeId uaNode = UA_NODEID_NUMERIC(1, 2);
    OpcUaNodeId nodeId(uaNode);
    UA_NodeId_clear(&uaNode);

    ASSERT_EQ(nodeId.getNamespaceIndex(), 1);
    ASSERT_EQ(nodeId.getIdentifierType(), OpcUaIdentifierType::Numeric);
    ASSERT_EQ(nodeId.getIdentifier(), "2");
}

TEST_F(OpcUaNodeIdTest, CreateFromOpcUaNodeString)
{
    UA_NodeId uaNode = UA_NODEID_STRING_ALLOC(1, "TestIden");
    OpcUaNodeId nodeId(uaNode);
    UA_NodeId_clear(&uaNode);

    ASSERT_EQ(nodeId.getNamespaceIndex(), 1);
    ASSERT_EQ(nodeId.getIdentifierType(), OpcUaIdentifierType::String);
    ASSERT_EQ(nodeId.getIdentifier(), "TestIden");
}

TEST_F(OpcUaNodeIdTest, CreateFromOpcUaNodeInt1)
{
    UA_NodeId uaNode = UA_NODEID_NUMERIC(1, 2);
    OpcUaNodeId nodeId = uaNode;
    UA_NodeId_clear(&uaNode);

    ASSERT_EQ(nodeId.getNamespaceIndex(), 1);
    ASSERT_EQ(nodeId.getIdentifierType(), OpcUaIdentifierType::Numeric);
    ASSERT_EQ(nodeId.getIdentifier(), "2");
}

TEST_F(OpcUaNodeIdTest, CreateFromOpcUaNodeString1)
{
    UA_NodeId uaNode = UA_NODEID_STRING_ALLOC(1, "TestIden");
    OpcUaNodeId nodeId = uaNode;
    UA_NodeId_clear(&uaNode);

    ASSERT_EQ(nodeId.getNamespaceIndex(), 1);
    ASSERT_EQ(nodeId.getIdentifierType(), OpcUaIdentifierType::String);
    ASSERT_EQ(nodeId.getIdentifier(), "TestIden");
}

TEST_F(OpcUaNodeIdTest, AppendStringToInt)
{
    OpcUaNodeId nodeId(1, 2);
    OpcUaNodeId newNode = nodeId.addSuffix("!!!");
    ASSERT_EQ(newNode.getNamespaceIndex(), 1);
    ASSERT_EQ(newNode.getIdentifierType(), OpcUaIdentifierType::String);
    ASSERT_EQ(newNode.getIdentifier(), "2!!!");

    newNode = nodeId.addSuffix("_", "!!!");
    ASSERT_EQ(newNode.getNamespaceIndex(), 1);
    ASSERT_EQ(newNode.getIdentifierType(), OpcUaIdentifierType::String);
    ASSERT_EQ(newNode.getIdentifier(), "2_!!!");
}

TEST_F(OpcUaNodeIdTest, AppendStringToString)
{
    OpcUaNodeId nodeId(1, "2");
    OpcUaNodeId newNode = nodeId.addSuffix("!!!");
    ASSERT_EQ(newNode.getNamespaceIndex(), 1);
    ASSERT_EQ(newNode.getIdentifierType(), OpcUaIdentifierType::String);
    ASSERT_EQ(newNode.getIdentifier(), "2!!!");

    newNode = nodeId.addSuffix("_", "!!!");
    ASSERT_EQ(newNode.getNamespaceIndex(), 1);
    ASSERT_EQ(newNode.getIdentifierType(), OpcUaIdentifierType::String);
    ASSERT_EQ(newNode.getIdentifier(), "2_!!!");
}

TEST_F(OpcUaNodeIdTest, TestEqual)
{
    OpcUaNodeId nodeIdStr1(1, "2");
    OpcUaNodeId nodeIdStr2(1, "2");
    OpcUaNodeId nodeIdStr3(1, "2222");
    OpcUaNodeId nodeIdStr4(2, "2");

    OpcUaNodeId nodeIdInt1(1, 2);
    OpcUaNodeId nodeIdInt2(1, 2);
    OpcUaNodeId nodeIdInt3(1, 3);
    OpcUaNodeId nodeIdInt4(2, 2);

    ASSERT_TRUE(nodeIdStr1 == nodeIdStr2);
    ASSERT_FALSE(nodeIdStr1 != nodeIdStr2);
    ASSERT_FALSE(nodeIdStr1 == nodeIdStr3);
    ASSERT_TRUE(nodeIdStr1 != nodeIdStr3);
    ASSERT_FALSE(nodeIdStr1 == nodeIdStr4);
    ASSERT_TRUE(nodeIdStr1 != nodeIdStr4);

    ASSERT_TRUE(nodeIdInt1 == nodeIdInt2);
    ASSERT_FALSE(nodeIdInt1 != nodeIdInt2);
    ASSERT_FALSE(nodeIdInt1 == nodeIdInt3);
    ASSERT_TRUE(nodeIdInt1 != nodeIdInt3);
    ASSERT_FALSE(nodeIdInt1 == nodeIdInt4);
    ASSERT_TRUE(nodeIdInt1 != nodeIdInt4);
}

TEST_F(OpcUaNodeIdTest, TestEqualUAType)
{
    OpcUaNodeId nodeIdStr1(1, "2");
    OpcUaNodeId nodeIdStr2(1, "2222");

    UA_NodeId uaNodeIdStr = UA_NODEID_STRING_ALLOC(1, "2222");

    ASSERT_TRUE(nodeIdStr1 != uaNodeIdStr);
    ASSERT_FALSE(nodeIdStr1 == uaNodeIdStr);

    ASSERT_FALSE(nodeIdStr2 != uaNodeIdStr);
    ASSERT_TRUE(nodeIdStr2 == uaNodeIdStr);

    UA_NodeId_clear(&uaNodeIdStr);
}

TEST_F(OpcUaNodeIdTest, TestLessOperator)
{
    OpcUaNodeId nodeIdStr1(1, "2");
    OpcUaNodeId nodeIdStr2(1, "2");
    OpcUaNodeId nodeIdStr3(1, "2222");
    OpcUaNodeId nodeIdStr4(2, "2");

    OpcUaNodeId nodeIdInt1(1, 2);
    OpcUaNodeId nodeIdInt2(1, 2);
    OpcUaNodeId nodeIdInt3(1, 3);
    OpcUaNodeId nodeIdInt4(2, 2);

    ASSERT_NE(nodeIdStr1 < nodeIdStr3, nodeIdStr3 < nodeIdStr1);
    ASSERT_NE(nodeIdStr1 < nodeIdStr4, nodeIdStr4 < nodeIdStr1);

    ASSERT_NE(nodeIdInt1 < nodeIdInt3, nodeIdInt3 < nodeIdInt1);
    ASSERT_NE(nodeIdInt1 < nodeIdInt4, nodeIdInt4 < nodeIdInt1);

    ASSERT_NE(nodeIdInt1 < nodeIdStr1, nodeIdStr1 < nodeIdInt1);

    ASSERT_FALSE(nodeIdInt1 < nodeIdInt1);
    ASSERT_FALSE(nodeIdInt1 < nodeIdInt2);
    ASSERT_FALSE(nodeIdInt2 < nodeIdInt1);

    ASSERT_FALSE(nodeIdStr1 < nodeIdStr1);
    ASSERT_FALSE(nodeIdStr1 < nodeIdStr2);
    ASSERT_FALSE(nodeIdStr2 < nodeIdStr1);
}

TEST_F(OpcUaNodeIdTest, TestToString)
{
    OpcUaNodeId nodeIdStr(1, "2");
    OpcUaNodeId nodeIdInt(1, 2);

    ASSERT_EQ(nodeIdStr.toString(), "(1, 2)");
    ASSERT_EQ(nodeIdInt.toString(), "(1, 2)");
}

TEST_F(OpcUaNodeIdTest, TestInstantiateNode)
{
    OpcUaNodeId nodeIdInt = OpcUaNodeId::instantiateNode(2, "4", OpcUaIdentifierType::Numeric);
    OpcUaNodeId nodeIdStr = OpcUaNodeId::instantiateNode(2, "4", OpcUaIdentifierType::String);

    ASSERT_EQ(nodeIdInt, OpcUaNodeId(2, 4));
    ASSERT_EQ(nodeIdStr, OpcUaNodeId(2, "4"));
}

TEST_F(OpcUaNodeIdTest, TestShallowCopy)
{
    OpcUaNodeId nodeid = OpcUaNodeId(UA_TYPES[UA_TYPES_INT64].typeId, true);
}

TEST_F(OpcUaNodeIdTest, HashTest)
{
    OpcUaNodeId nodeIdStr1(1, "str1");
    OpcUaNodeId nodeIdStr2(1, "str2");
    OpcUaNodeId nodeIdStr3(1, "str2");

    OpcUaNodeId nodeIdInt1(1, 2);
    OpcUaNodeId nodeIdInt2(1, 2);

    ASSERT_NE(std::hash<OpcUaNodeId>{}(nodeIdStr1), std::hash<OpcUaNodeId>{}(nodeIdStr2));
    ASSERT_EQ(std::hash<OpcUaNodeId>{}(nodeIdStr2), std::hash<OpcUaNodeId>{}(nodeIdStr3));
    ASSERT_NE(std::hash<OpcUaNodeId>{}(nodeIdStr1), std::hash<OpcUaNodeId>{}(nodeIdInt1));
    ASSERT_EQ(std::hash<OpcUaNodeId>{}(nodeIdInt1), std::hash<OpcUaNodeId>{}(nodeIdInt2));
}

TEST_F(OpcUaNodeIdTest, CreateWithRandomGuid)
{
    OpcUaNodeId nodeId = OpcUaNodeId::CreateWithRandomGuid();
    ASSERT_EQ(nodeId.getIdentifierType(), OpcUaIdentifierType::Guid);
}

TEST_F(OpcUaNodeIdTest, TestIdentifierNumeric)
{
    auto node1 = OpcUaNodeId::instantiateNode(1, "-999", OpcUaIdentifierType::Numeric);
    ASSERT_EQ(node1.getIdentifier(), "0");

    auto node2 = OpcUaNodeId::instantiateNode(1, "4259502693", OpcUaIdentifierType::Numeric);
    ASSERT_EQ(node2.getIdentifier(), "4259502693");

    auto node3 = OpcUaNodeId::instantiateNode(1, " 756", OpcUaIdentifierType::Numeric);
    ASSERT_EQ(node3.getIdentifier(), "0");

    auto node4 = OpcUaNodeId::instantiateNode(1, "ab 756 t 89", OpcUaIdentifierType::Numeric);
    ASSERT_EQ(node4.getIdentifier(), "0");

    auto node6 = OpcUaNodeId::instantiateNode(1, " 3", OpcUaIdentifierType::Numeric);
    ASSERT_EQ(node6.getIdentifier(), "0");
}

END_NAMESPACE_OPENDAQ_OPCUA
