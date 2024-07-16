#include <gtest/gtest.h>
#include <opcuashared/opcuaendpoint.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaEndpointTest = testing::Test;

TEST_F(OpcUaEndpointTest, Create)
{
    auto endpoint = OpcUaEndpoint("opc.tcp://localhost:4840");
    ASSERT_EQ(endpoint.getUrl(), "opc.tcp://localhost:4840");
}

TEST_F(OpcUaEndpointTest, CreateUsernamePassword)
{
    auto endpoint = OpcUaEndpoint("opc.tcp://127.0.0.1", "un", "pass");

    ASSERT_EQ(endpoint.getUrl(), "opc.tcp://127.0.0.1");
    ASSERT_EQ(endpoint.getUsername(), "un");
    ASSERT_EQ(endpoint.getPassword(), "pass");
}

TEST_F(OpcUaEndpointTest, SettersAndGetters)
{
    auto endpoint = OpcUaEndpoint("opc.tcp://localhost:4840");

    endpoint.setUrl("opc.tcp://localhost:2000");
    ASSERT_EQ(endpoint.getUrl(), "opc.tcp://localhost:2000");

    endpoint.setName("name");
    ASSERT_EQ(endpoint.getName(), "name");

    endpoint.setUsername("username");
    ASSERT_EQ(endpoint.getUsername(), "username");

    endpoint.setPassword("123");
    ASSERT_EQ(endpoint.getPassword(), "123");
}

TEST_F(OpcUaEndpointTest, RegisterCustomTypes)
{
    auto endpoint = OpcUaEndpoint("opc.tcp://127.0.0.1:4840");

    endpoint.registerCustomTypes(UA_TYPES_COUNT, UA_TYPES);
    auto typeList = endpoint.getCustomDataTypes();

    ASSERT_EQ(typeList[0].typesSize, static_cast<std::size_t>(UA_TYPES_COUNT));
    ASSERT_EQ(typeList[0].types, UA_TYPES);
}

TEST_F(OpcUaEndpointTest, IsAnonymous)
{
    auto endpoint = OpcUaEndpoint("opc.tcp://127.0.0.1:4840");
    ASSERT_TRUE(endpoint.isAnonymous());

    endpoint.setUsername("u");
    ASSERT_FALSE(endpoint.isAnonymous());

    endpoint.setUsername("");
    endpoint.setPassword("u");
    ASSERT_TRUE(endpoint.isAnonymous());
}

END_NAMESPACE_OPENDAQ_OPCUA
