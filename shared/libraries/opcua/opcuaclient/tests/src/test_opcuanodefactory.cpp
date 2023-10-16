#include <testutils/testutils.h>
#include "opcuaservertesthelper.h"
#include <opcuaclient/opcuanodefactory.h>
#include <opcuashared/node/opcuadatatype.h>
#include <opcuashared/node/opcuanodeobject.h>
#include <opcuashared/node/opcuanodevariable.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

#if IGNORE_FOR_DEVELOPMENT <= 0

using OpcUaNodeFactoryTest = BaseClientTest;

static OpcUaObject<UA_ReferenceDescription> PrepareReferenceDescription(OpcUaNodeClass nodeClass, const std::string& name, const OpcUaNodeId& nodeId, const OpcUaNodeId& typeId)
{
    OpcUaObject<UA_ReferenceDescription> desc;
    desc->nodeClass = static_cast<UA_NodeClass>(nodeClass);
    desc->browseName = UA_QUALIFIEDNAME_ALLOC(1, name.c_str());
    desc->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", name.c_str());
    desc->nodeId = UA_EXPANDEDNODEID_NODEID(nodeId.copyAndGetDetachedValue());
    desc->typeDefinition = UA_EXPANDEDNODEID_NODEID(typeId.copyAndGetDetachedValue());
    return desc;
}

TEST_F_OPTIONAL(OpcUaNodeFactoryTest, Create)
{
    auto client = prepareAndConnectClient();
    ASSERT_NO_THROW(OpcUaNodeFactory factory(client));
}

TEST_F_OPTIONAL(OpcUaNodeFactoryTest, InstatinateObject)
{
    auto client = prepareAndConnectClient();
    OpcUaNodeFactory factory(client);

    auto referenceDescription = PrepareReferenceDescription(OpcUaNodeClass::Object, "f1", OpcUaNodeId(1, "f1"), OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE)));

    bool traverse;
    auto node = factory.instantiateNode(*referenceDescription, OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER)), traverse);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getBrowseName(), "f1");
    ASSERT_EQ(node->getDisplayName(), "f1");
    ASSERT_EQ(node->getNodeId(), OpcUaNodeId(1, "f1"));

    auto nodeObject = std::dynamic_pointer_cast<OpcUaNodeObject>(node);
    ASSERT_NE(nodeObject, nullptr);
}

TEST_F_OPTIONAL(OpcUaNodeFactoryTest, InstatinateVariable)
{
    auto client = prepareAndConnectClient();
    OpcUaNodeFactory factory(client);

    auto referenceDescription = PrepareReferenceDescription(OpcUaNodeClass::Variable, ".i32", OpcUaNodeId(1, ".i32"), OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE)));

    bool traverse;
    auto node = factory.instantiateNode(*referenceDescription, OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER)), traverse);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getBrowseName(), ".i32");
    ASSERT_EQ(node->getDisplayName(), ".i32");
    ASSERT_EQ(node->getNodeId(), OpcUaNodeId(1, ".i32"));

    auto nodeValue = std::dynamic_pointer_cast<OpcUaNodeVariable>(node);
    ASSERT_NE(nodeValue, nullptr);
    ASSERT_EQ(nodeValue->getDataTypeNodeId(), UA_TYPES[UA_TYPES_INT32].typeId);
}

TEST_F_OPTIONAL(OpcUaNodeFactoryTest, InstatinateMethod)
{
    auto client = prepareAndConnectClient();
    OpcUaNodeFactory factory(client);

    auto referenceDescription = PrepareReferenceDescription(OpcUaNodeClass::Method, "hello.dewesoft", OpcUaNodeId(1, "hello.dewesoft"), OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_METHODATTRIBUTES))); // TODO TYPES

    bool traverse;
    auto node = factory.instantiateNode(*referenceDescription, OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER)), traverse);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getBrowseName(), "hello.dewesoft");
    ASSERT_EQ(node->getDisplayName(), "hello.dewesoft");
    ASSERT_EQ(node->getNodeId(), OpcUaNodeId(1, "hello.dewesoft"));

    auto nodeMethod = std::dynamic_pointer_cast<OpcUaNodeMethod>(node);
    ASSERT_NE(nodeMethod, nullptr);
    ASSERT_EQ(nodeMethod->inputParameters.size(), 1u);
    ASSERT_EQ(nodeMethod->outputParameters.size(), 1u);
}

TEST_F_OPTIONAL(OpcUaNodeFactoryTest, InstatinateDataType)
{
    auto client = prepareAndConnectClient();
    OpcUaNodeFactory factory(client);

    auto referenceDescription = PrepareReferenceDescription(OpcUaNodeClass::DataType, "Boolean", OpcUaNodeId(0, 1), OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE)));

    bool traverse;
    auto node = factory.instantiateNode(*referenceDescription, OpcUaNodeId(UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER)), traverse);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->getBrowseName(), "Boolean");
    ASSERT_EQ(node->getDisplayName(), "Boolean");
    ASSERT_EQ(node->getNodeId(), OpcUaNodeId(0, 1));

    auto dataType = std::dynamic_pointer_cast<OpcUaDataType>(node);
    ASSERT_NE(dataType, nullptr);
}

#endif

END_NAMESPACE_OPENDAQ_OPCUA
