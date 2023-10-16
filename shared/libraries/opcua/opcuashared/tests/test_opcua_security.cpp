#include "gtest/gtest.h"
#include "testutils/testutils.h"
#include <opcuashared/opcuaendpoint.h>
#include <opcuaclient/opcuaclient.h>
#include <opcuaserver/opcuaserver.h>
#include <opcuaserver/node/opcuanodevalueservert.h>
#include <opcuashared/generated/cmake_globals.h>
#include <map>
#include <opcuashared/opcuaobject.h>
#include <thread>
#include <commonlib/crypto/bcrypt/bcrypt.h>

using Dewesoft::Utils::Crypto::BCrypt;

BEGIN_NAMESPACE_OPCUA

class OpcUaSecurityTest : public MemCheckTest
{
    bool EnableMemoryLeakDump()
    {
        return false;
    }
};

static std::string TestFile(std::string name)
{
    return CmakeGlobals::ResDirFile("testKeys/keys/" + name);
}

class TestServer
{
public:
    TestServer()
    {
        server = std::make_shared<OpcUaServer>();
        server->setPort(4840);
    }

    ~TestServer()
    {
        if (isStarted())
            server->stop();
        nodes.clear();
    }

    void setSecurityConfig(OpcUaServerSecurityConfig* config)
    {
        server->setSecurityConfig(config);
    }

    void defineStringVar(std::string name, std::string value)
    {
        UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(1, name.c_str());
        UA_String valueStr = UA_STRING_ALLOC(value.c_str());

        OpcUaNodeValueServerStringPtr node = std::make_shared<OpcUaNodeValueServerString>(nodeId);
        node->setBrowseName(name.c_str());
        node->setValue(valueStr, UA_DateTime_now());
        server->createServerNode(node, OpcUaNodeId(0, UA_NS0ID_SERVER));
        nodes.push_back(node);

        UA_String_clear(&valueStr);
        UA_NodeId_clear(&nodeId);
    }

    void start()
    {
        server->start();
    }

    void stop()
    {
        server->stop();
    }

    bool isStarted()
    {
        return server->getStarted();
    }

    std::shared_ptr<OpcUaServer> getServer()
    {
        return server;
    }

    static OpcUaServerSecurityConfig CreateSecurityConfig()
    {
        OpcUaServerSecurityConfig config;
        config.certificate = OpcUaCommon::loadFile(TestFile("server-cert.der"));
        config.privateKey = OpcUaCommon::loadFile(TestFile("server-private.der"));
        config.trustList.push_back(OpcUaCommon::loadFile(TestFile("client-cert.der")).getValue());
        config.appUri = "urn:dewesoft.com";
        config.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        return config;
    }

    static std::unordered_map<std::string, std::string> CreateUsers()
    {
        BCrypt bcrypt;
        unsigned int rounds = 8;
        std::unordered_map<std::string, std::string> users;
        users["newton"] = bcrypt.hash("newton123", rounds);
        users["galileo"] = bcrypt.hash("galileo123", rounds);
        users["tesla"] = bcrypt.hash("tesla123", rounds);
        users["pascal"] = bcrypt.hash("pascal123", rounds);
        return users;
    }

private:
    std::shared_ptr<OpcUaServer> server;
    std::vector<OpcUaNodePtr> nodes;
};

class TestClient
{
public:
    TestClient()
    {
        client = std::make_shared<OpcUaClient>(OpcUaDataFetchStrategy::Buffered);
        securityConfig = NULL;
    }

    void setSecurityConfig(OpcUaClientSecurityConfig* config)
    {
        securityConfig = config;
    }

    void connect()
    {
        OpcUaEndpoint endpoint("test", "opc.tcp://localhost:4840/", OpcUaServerType::General, "");
        endpoint.setSecurityConfig(securityConfig);

        OpcUaCollection<OpcUaEndpoint> endpoints;
        endpoints.push_back(endpoint);
        client->assignEndpoints(endpoints);

        client->connect();
    }

    void disconnect()
    {
        client->disconnect();
    }

    bool isConnected()
    {
        return client->size() > 0 && client->at(0)->isConnected();
    }

    std::string readStringVar(std::string name)
    {
        UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(1, name.c_str());
        ReadResult r = client->at(0)->readValue(&nodeId);

        UA_Variant variant = r.value->getValue();
        UA_String* uaStr = (UA_String*) variant.data;
        std::string value = std::string((char*) uaStr->data, uaStr->length);

        UA_NodeId_clear(&nodeId);
        return value;
    }

    std::shared_ptr<OpcUaClient> getClient()
    {
        return client;
    }

    static OpcUaClientSecurityConfig CreateSecurityConfig()
    {
        OpcUaClientSecurityConfig config;
        config.certificate = OpcUaCommon::loadFile(TestFile("client-cert.der"));
        config.privateKey = OpcUaCommon::loadFile(TestFile("client-private.der"));
        config.trustList.push_back(OpcUaCommon::loadFile(TestFile("server-cert.der")).getValue());
        config.appUri = "urn:testclient.com";
        config.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        return config;
    }

private:
    std::shared_ptr<OpcUaClient> client;
    OpcUaClientSecurityConfig* securityConfig;
};

TEST_F(OpcUaSecurityTest, LoadCertificateTest)
{
    OpcUaObject<UA_ByteString> cert;

    ASSERT_NO_THROW(cert = OpcUaCommon::loadFile(TestFile("client-cert.der")));
    ASSERT_GT(cert.getValue().length, 0);

    ASSERT_ANY_THROW(cert = OpcUaCommon::loadFile(TestFile("client-cert-missing.der")));

    ASSERT_NO_THROW(cert = OpcUaCommon::loadFile(TestFile("garbage-cert.der")));
    ASSERT_GT(cert.getValue().length, 0);
}

TEST_F(OpcUaSecurityTest, SecurityConfigMemoryTest)
{
    OpcUaClientSecurityConfig a;
    a.certificate = OpcUaCommon::loadFile(TestFile("client-cert.der")).getValue();

    a.trustList.push_back(OpcUaCommon::loadFile(TestFile("server-cert.der")).getValue());
    a.trustList[0] = OpcUaCommon::loadFile(TestFile("client-cert.der")).getValue();

    a.appUri = "urn:testclient.com";
    a.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    OpcUaClientSecurityConfig b = a;
}

TEST_F(OpcUaSecurityTest, PlainTextTest)
{
    std::string message = "Hello world, plain text communication works.";

    TestServer testServer;
    testServer.setSecurityConfig(NULL);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    testServer.defineStringVar("testVar", message);

    TestClient testClient;
    testClient.setSecurityConfig(NULL);
    testClient.connect();
    ASSERT_TRUE(testClient.isConnected());

    std::string received = testClient.readStringVar("testVar");
    ASSERT_EQ(received, message);

    testClient.disconnect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F(OpcUaSecurityTest, SecurityModeNoneTest)
{
    std::string message = "Hello world, security mode UA_MESSAGESECURITYMODE_NONE works.";

    OpcUaServerSecurityConfig serverSecurity;
    serverSecurity.securityMode = UA_MESSAGESECURITYMODE_NONE;

    OpcUaClientSecurityConfig clientSecurity;
    clientSecurity.securityMode = UA_MESSAGESECURITYMODE_NONE;

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    testServer.defineStringVar("testVar", message);

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);
    testClient.connect();
    ASSERT_TRUE(testClient.isConnected());

    std::string received = testClient.readStringVar("testVar");
    ASSERT_EQ(received, message);

    testClient.disconnect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F(OpcUaSecurityTest, SecurityModeSignEncryptTest)
{
    std::string message = "Hello world, security mode UA_MESSAGESECURITYMODE_SIGNANDENCRYPT works.";

    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    OpcUaClientSecurityConfig clientSecurity = TestClient::CreateSecurityConfig();

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    testServer.defineStringVar("testVar", message);

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);
    testClient.connect();
    ASSERT_TRUE(testClient.isConnected());

    std::string received = testClient.readStringVar("testVar");
    ASSERT_EQ(received, message);

    testClient.disconnect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F(OpcUaSecurityTest, WrongSecurityModeTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    serverSecurity.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    OpcUaClientSecurityConfig clientSecurity = TestClient::CreateSecurityConfig();
    clientSecurity.securityMode = UA_MESSAGESECURITYMODE_NONE;

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F(OpcUaSecurityTest, ExpiredCertificateTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    serverSecurity.trustList.push_back(OpcUaCommon::loadFile(TestFile("client-cert-expired.der")).getValue());

    OpcUaClientSecurityConfig clientSecurity;
    clientSecurity.trustList.push_back(OpcUaCommon::loadFile(TestFile("server-cert.der")).getValue());
    clientSecurity.certificate = OpcUaCommon::loadFile(TestFile("client-cert-expired.der"));
    clientSecurity.privateKey = OpcUaCommon::loadFile(TestFile("client-private-expired.der"));
    clientSecurity.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    clientSecurity.appUri = "urn:testclient.com";

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F(OpcUaSecurityTest, GarbageCertificateTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();

    OpcUaClientSecurityConfig clientSecurity;
    clientSecurity.trustList.push_back(OpcUaCommon::loadFile(TestFile("server-cert.der")).getValue());
    clientSecurity.certificate = OpcUaCommon::loadFile(TestFile("garbage-cert.der")).getValue();
    clientSecurity.privateKey = OpcUaCommon::loadFile(TestFile("client-private.der")).getValue();
    clientSecurity.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    clientSecurity.appUri = "urn:testclient.com";

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    serverSecurity.trustList.push_back(OpcUaCommon::loadFile(TestFile("garbage-cert.der")).getValue());
    ASSERT_THROW(testServer.start(), OpcUaException);
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F(OpcUaSecurityTest, UntrustedCertificateTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();

    OpcUaClientSecurityConfig clientSecurity;
    clientSecurity.trustList.push_back(OpcUaCommon::loadFile(TestFile("server-cert.der")).getValue());
    clientSecurity.certificate = OpcUaCommon::loadFile(TestFile("tesla-cert.der"));
    clientSecurity.privateKey = OpcUaCommon::loadFile(TestFile("tesla-private.der"));
    clientSecurity.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    clientSecurity.appUri = "urn:nikolatesla.com";

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    serverSecurity.trustList.push_back(OpcUaCommon::loadFile(TestFile("tesla-cert.der")).getValue());
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());
    testClient.connect();

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F(OpcUaSecurityTest, AuthenticationTest)
{
    std::unordered_map<std::string, std::string> users = TestServer::CreateUsers();

    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    serverSecurity.authenticateUser = [&users](bool isAnonymous, std::string username, std::string password) -> UA_StatusCode {
        if (!isAnonymous && users.count(username))
        {
            std::string hash = users[username];
            if (BCrypt::Verify(hash, password))
                return UA_STATUSCODE_GOOD;
        }

        return UA_STATUSCODE_BADUSERACCESSDENIED;
    };

    OpcUaClientSecurityConfig clientSecurity = TestClient::CreateSecurityConfig();

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);

    // annonymous login
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    // wrong password
    clientSecurity.username = "tesla";
    clientSecurity.password = "wrongPassword";
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    // wrong user
    clientSecurity.username = "wrongUser";
    clientSecurity.password = "tesla123";
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    // crrect login
    clientSecurity.username = "tesla";
    clientSecurity.password = "tesla123";
    testClient.connect();
    ASSERT_TRUE(testClient.isConnected());

    testClient.disconnect();
    ASSERT_FALSE(testClient.isConnected());
    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

END_NAMESPACE_OPCUA
