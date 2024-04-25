#include <gtest/gtest.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuaserver/opcuaserver.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>

using namespace daq;
using namespace daq::opcua;

using OpcUaAuthenticationTest = testing::Test;

TEST_F(OpcUaAuthenticationTest, DefaultConnect)
{
    auto server = OpcUaServer();
    server.start();

    auto client = OpcUaClient("opc.tcp://127.0.0.1");
    client.connect();
    ASSERT_TRUE(client.isConnected());
}

TEST_F(OpcUaAuthenticationTest, NoAnonymous)
{
    auto server = OpcUaServer();
    server.setPort(4840);
    server.setAuthenticationProvider(AuthenticationProvider(false));
    server.start();

    auto client = OpcUaClient("opc.tcp://127.0.0.1");
    client.connect();
    ASSERT_FALSE(client.isConnected());
}

TEST_F(OpcUaAuthenticationTest, AuthenticationProvider)
{
    auto users = List<IUser>();
    users.pushBack(User("jure", "jure123"));
    users.pushBack(User("tomaz", "tomaz123"));
    auto authenticationProvider = StaticAuthenticationProvider(true, users);

    auto server = OpcUaServer();
    server.setPort(4840);
    server.setAuthenticationProvider(authenticationProvider);
    server.start();

    OpcUaClientPtr client;

    client = std::make_shared<OpcUaClient>("opc.tcp://127.0.0.1");
    ASSERT_TRUE(client->connect());

    client = std::make_shared<OpcUaClient>(OpcUaEndpoint("opc.tcp://127.0.0.1", "jure", "wrongPass"));
    ASSERT_FALSE(client->connect());

    client = std::make_shared<OpcUaClient>(OpcUaEndpoint("opc.tcp://127.0.0.1", "jure", "jure123"));
    ASSERT_TRUE(client->connect());

    client = std::make_shared<OpcUaClient>(OpcUaEndpoint("opc.tcp://127.0.0.1", "tomaz", "tomaz123"));
    ASSERT_TRUE(client->connect());
}
