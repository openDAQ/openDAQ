#include <gtest/gtest.h>
#include <opcuatms_client/objects/tms_client_context.h>

using namespace daq::opcua;
using namespace daq::opcua::tms;
using namespace daq;

using ClientContextTest = testing::Test;

static OpcUaClientPtr CreateClient()
{
    return std::make_shared<OpcUaClient>("opc.tcp://localhost");
}

TEST_F(ClientContextTest, Create)
{
    auto client = CreateClient();
    ASSERT_NO_THROW(TmsClientContext clientContext(client));
}

TEST_F(ClientContextTest, RegisterObject)
{
    auto client = CreateClient();
    TmsClientContext clientContext(client);

    StringPtr str = "TestStrObject";
    ASSERT_NO_THROW(clientContext.registerObject(OpcUaNodeId(1, 1), str));
}

TEST_F(ClientContextTest, ContextGetObject)
{
    auto client = CreateClient();
    TmsClientContext clientContext(client);

    BaseObjectPtr baseObj;
    ASSERT_NO_THROW(baseObj = clientContext.getObject(OpcUaNodeId(1, 1)));
    ASSERT_FALSE(baseObj.assigned());

    StringPtr str = "TestStrObject";
    clientContext.registerObject(OpcUaNodeId(1, 1), str);

    ASSERT_NO_THROW(baseObj = clientContext.getObject(OpcUaNodeId(1, 1)));
    ASSERT_TRUE(baseObj.assigned());
    ASSERT_EQ(baseObj, str);
}

TEST_F(ClientContextTest, ContextGetObjectTemplate)
{
    auto client = CreateClient();
    TmsClientContext clientContext(client);

    StringPtr strObj;
    ASSERT_NO_THROW(strObj = clientContext.getObject<IString>(OpcUaNodeId(1, 1)));
    ASSERT_FALSE(strObj.assigned());

    StringPtr str = "TestStrObject";
    clientContext.registerObject(OpcUaNodeId(1, 1), str);

    ASSERT_NO_THROW(strObj = clientContext.getObject<IString>(OpcUaNodeId(1, 1)));
    ASSERT_TRUE(strObj.assigned());
    ASSERT_EQ(strObj, str);

    FloatPtr floatPtr;
    ASSERT_NO_THROW(floatPtr = clientContext.getObject<IFloat>(OpcUaNodeId(1, 1)));
    ASSERT_FALSE(floatPtr.assigned());
}

TEST_F(ClientContextTest, UnregisterObject)
{
    auto client = CreateClient();
    TmsClientContext clientContext(client);

    ASSERT_NO_THROW(clientContext.unregisterObject(OpcUaNodeId(1, 1)));

    StringPtr str = "TestStrObject";
    clientContext.registerObject(OpcUaNodeId(1, 1), str);

    ASSERT_NO_THROW(clientContext.unregisterObject(OpcUaNodeId(1, 1)));

    auto baseObj = clientContext.getObject(OpcUaNodeId(1, 1));
    ASSERT_FALSE(baseObj.assigned());
}

TEST_F(ClientContextTest, TestRefCount)
{
    auto getRefCount = [](const StringPtr& obj)
    {
        obj->addRef();
        return obj->releaseRef();
    };

    auto client = CreateClient();
    TmsClientContext clientContext(client);

    StringPtr str = "TestStrObject";
    ASSERT_EQ(getRefCount(str), 1);

    clientContext.registerObject(OpcUaNodeId(1, 1), str);
    ASSERT_EQ(getRefCount(str), 1);
}
