#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <opcuashared/opcuaendpoint.h>
#include <opcuaserver/opcuaserver.h>
#include <opcuashared/generated/cmake_globals.h>
#include <map>
#include <opcuashared/opcuaobject.h>
#include <opcuashared/bcrypt.h>
#include "opcuashared/opcuasecuritycommon.h"
#include <opcuaclient/opcuaclient.h>

using namespace daq::utils;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaSecurityTest = testing::Test;

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
        config.certificate = utils::LoadFile(TestFile("server-cert.der"));
        config.privateKey = utils::LoadFile(TestFile("server-private.der"));
        config.trustList.push_back(utils::LoadFile(TestFile("client-cert.der")).getValue());
        config.appUri = "urn:dewesoft.com";
        config.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        config.trustAll = false;
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
        securityConfig = NULL;
    }

    void setSecurityConfig(OpcUaClientSecurityConfig* config)
    {
        securityConfig = config;
    }

    void connect()
    {
        OpcUaEndpoint endpoint("opc.tcp://localhost:4840/");
        //endpoint.setSecurityConfig(securityConfig);

        client = std::make_shared<OpcUaClient>(endpoint);
        
        if (timeout > 0)
            client->setTimeout(timeout);

        client->connect();
    }

    void disconnect()
    {
        client->disconnect();
        client = nullptr;
    }

    void setTimeout(int timeoutMs)
    {
        this->timeout = timeoutMs;
    }

    bool isConnected()
    {
        return client && client->isConnected();
    }

    bool nodeExist(const OpcUaNodeId& nodeId)
    {
        auto result = client->nodeExists(nodeId);
        return result;
    }

    OpcUaClientPtr getClient()
    {
        return client;
    }

    static OpcUaClientSecurityConfig CreateSecurityConfig()
    {
        OpcUaClientSecurityConfig config;
        config.certificate = utils::LoadFile(TestFile("client-cert.der"));
        config.privateKey = utils::LoadFile(TestFile("client-private.der"));
        config.trustList.push_back(utils::LoadFile(TestFile("server-cert.der")).getValue());
        config.appUri = "urn:testclient.com";
        config.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        config.trustAll = false;
        return config;
    }

private:
    OpcUaClientPtr client;
    OpcUaClientSecurityConfig* securityConfig;
    int timeout = -1;
};

TEST_F_OPTIONAL(OpcUaSecurityTest, LoadCertificateTest)
{
    OpcUaObject<UA_ByteString> cert;

    ASSERT_NO_THROW(cert = utils::LoadFile(TestFile("client-cert.der")));
    ASSERT_GT(cert.getValue().length, (size_t) 0);

    ASSERT_ANY_THROW(cert = utils::LoadFile(TestFile("client-cert-missing.der")));

    ASSERT_NO_THROW(cert = utils::LoadFile(TestFile("garbage-cert.der")));
    ASSERT_GT(cert.getValue().length, (size_t) 0);
}

TEST_F_OPTIONAL(OpcUaSecurityTest, SecurityConfigMemoryTest)
{
    OpcUaClientSecurityConfig a;
    a.certificate = utils::LoadFile(TestFile("client-cert.der")).getValue();

    a.trustList.push_back(utils::LoadFile(TestFile("server-cert.der")).getValue());
    a.trustList[0] = utils::LoadFile(TestFile("client-cert.der")).getValue();

    a.appUri = "urn:testclient.com";
    a.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    OpcUaClientSecurityConfig b = a;
}

TEST_F_OPTIONAL(OpcUaSecurityTest, SecurityModeNoneTest)
{
    OpcUaServerSecurityConfig serverSecurity;
    serverSecurity.securityMode = UA_MESSAGESECURITYMODE_NONE;

    OpcUaClientSecurityConfig clientSecurity;
    clientSecurity.securityMode = UA_MESSAGESECURITYMODE_NONE;

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);
    testClient.connect();
    ASSERT_TRUE(testClient.isConnected());

    ASSERT_NO_THROW(testClient.nodeExist(OpcUaNodeId(UA_NS0ID_SERVER)));
    
    testClient.disconnect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F_OPTIONAL(OpcUaSecurityTest, AuthenticationTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    OpcUaClientSecurityConfig clientSecurity = TestClient::CreateSecurityConfig();

#ifndef OPCUA_ENABLE_ENCRYPTION
    serverSecurity.certificate = UA_BYTESTRING_NULL;
    serverSecurity.securityMode = UA_MESSAGESECURITYMODE_NONE;
    clientSecurity.certificate = UA_BYTESTRING_NULL;
    clientSecurity.securityMode = UA_MESSAGESECURITYMODE_NONE;
#endif

    std::unordered_map<std::string, std::string> users = TestServer::CreateUsers();
    serverSecurity.authenticateUser = [&users](bool isAnonymous, std::string username, std::string password) -> UA_StatusCode {
        if (!isAnonymous && users.count(username))
        {
            std::string hash = users[username];
            if (BCrypt::Verify(hash, password))
                return UA_STATUSCODE_GOOD;
        }

        return UA_STATUSCODE_BADUSERACCESSDENIED;
    };

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);

    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    clientSecurity.username = "tesla";
    clientSecurity.password = "wrongPassword";
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    clientSecurity.username = "wrongUser";
    clientSecurity.password = "tesla123";
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    clientSecurity.username.reset();
    clientSecurity.password = "tesla123";
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    clientSecurity.username = "";
    clientSecurity.password = "tesla123";
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    clientSecurity.username = "tesla";
    clientSecurity.password = "";
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    clientSecurity.username = "tesla";
    clientSecurity.password.reset();
    testClient.connect();
    ASSERT_FALSE(testClient.isConnected());

    clientSecurity.username = "tesla";
    clientSecurity.password = "tesla123";
    testClient.connect();
    ASSERT_TRUE(testClient.isConnected());

    testClient.disconnect();
    ASSERT_FALSE(testClient.isConnected());
    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

#ifdef OPCUA_ENABLE_ENCRYPTION

TEST_F_OPTIONAL(OpcUaSecurityTest, SecurityModeSignEncryptTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    OpcUaClientSecurityConfig clientSecurity = TestClient::CreateSecurityConfig();

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    TestClient testClient;
    testClient.setSecurityConfig(&clientSecurity);
    testClient.connect();
    ASSERT_TRUE(testClient.isConnected());

    ASSERT_NO_THROW(testClient.nodeExist(UA_NS0ID_SERVER));

    testClient.disconnect();
    ASSERT_FALSE(testClient.isConnected());

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F_OPTIONAL(OpcUaSecurityTest, WrongSecurityModeTest)
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

TEST_F_OPTIONAL(OpcUaSecurityTest, ExpiredCertificateTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    serverSecurity.trustList.push_back(LoadFile(TestFile("client-cert-expired.der")).getValue());

    OpcUaClientSecurityConfig clientSecurity;
    clientSecurity.trustList.push_back(LoadFile(TestFile("server-cert.der")).getValue());
    clientSecurity.certificate = LoadFile(TestFile("client-cert-expired.der"));
    clientSecurity.privateKey = LoadFile(TestFile("client-private-expired.der"));
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

TEST_F_OPTIONAL(OpcUaSecurityTest, GarbageCertificateTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();

    OpcUaClientSecurityConfig clientSecurity;
    clientSecurity.trustList.push_back(LoadFile(TestFile("server-cert.der")).getValue());
    clientSecurity.certificate = LoadFile(TestFile("garbage-cert.der")).getValue();
    clientSecurity.privateKey = LoadFile(TestFile("client-private.der")).getValue();
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
    serverSecurity.trustList.push_back(LoadFile(TestFile("garbage-cert.der")).getValue());
    testServer.setSecurityConfig(&serverSecurity);
    ASSERT_THROW(testServer.start(), OpcUaException);
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F_OPTIONAL(OpcUaSecurityTest, UntrustedCertificateTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();

    OpcUaClientSecurityConfig clientSecurity;
    clientSecurity.trustList.push_back(LoadFile(TestFile("server-cert.der")).getValue());
    clientSecurity.certificate = LoadFile(TestFile("tesla-cert.der"));
    clientSecurity.privateKey = LoadFile(TestFile("tesla-private.der"));
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
    serverSecurity.trustList.push_back(LoadFile(TestFile("tesla-cert.der")).getValue());
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());
    testClient.connect();

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F_OPTIONAL(OpcUaSecurityTest, RevokedCertificateTest)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    serverSecurity.trustList.clear();
    serverSecurity.trustList.push_back(LoadFile(TestFile("client-cert.der")).getValue());
    serverSecurity.trustList.push_back(LoadFile(TestFile("tesla-cert.der")).getValue());
    serverSecurity.revocationList.clear();
    serverSecurity.revocationList.push_back(LoadFile(TestFile("../ca/root.crl")).getValue());

    auto tryConnectClient = [](std::string prefix) {
        OpcUaClientSecurityConfig clientSecurity = TestClient::CreateSecurityConfig();
        clientSecurity.certificate = LoadFile(TestFile(prefix + "-cert.der"));
        clientSecurity.privateKey = LoadFile(TestFile(prefix + "-private.der"));

        TestClient client;
        client.setSecurityConfig(&clientSecurity);
        client.connect();
        bool connected = client.isConnected();
        client.disconnect();
        return connected;
    };

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());

    ASSERT_TRUE(tryConnectClient("client"));
    ASSERT_FALSE(tryConnectClient("tesla")); // todo: fix revocation list issue

    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F_OPTIONAL(OpcUaSecurityTest, AppUriFromCertificate)
{
    OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
    serverSecurity.appUri.reset();

    TestServer testServer;
    testServer.setSecurityConfig(&serverSecurity);
    testServer.start();
    ASSERT_TRUE(testServer.isStarted());
    testServer.stop();
    ASSERT_FALSE(testServer.isStarted());
}

TEST_F_OPTIONAL(OpcUaSecurityTest, ParseCertificateUri)
{
    OpcUaObject<UA_ByteString> serverCert = LoadFile(TestFile("server-cert.der"));
    OpcUaObject<UA_ByteString> clientCert = LoadFile(TestFile("client-cert.der"));
    OpcUaObject<UA_ByteString> garbageCert = LoadFile(TestFile("garbage-cert.der"));

    std::optional<std::string> urn;
    urn = OpcUaSecurityCommon::parseCertificateUri(serverCert.getValue());
    ASSERT_EQ(urn.value_or(""), "urn:dewesoft.com");
    urn = OpcUaSecurityCommon::parseCertificateUri(clientCert.getValue());
    ASSERT_EQ(urn.value_or(""), "urn:testclient.com");
    urn = OpcUaSecurityCommon::parseCertificateUri(garbageCert.getValue());
    ASSERT_EQ(urn.value_or(""), "");
}

TEST_F_OPTIONAL(OpcUaSecurityTest, TrustAllTest)
{
    OpcUaObject<UA_ByteString> clientCert = LoadFile(TestFile("client-cert.der"));
    OpcUaObject<UA_ByteString> serverCert = LoadFile(TestFile("server-cert.der"));

    auto tryConnect = [](bool serverTrustAll,
                         std::vector<UA_ByteString> serverTrustList,
                         bool clientTrustAll,
                         std::vector<UA_ByteString> clientTrustList) {
        OpcUaServerSecurityConfig serverSecurity = TestServer::CreateSecurityConfig();
        serverSecurity.trustAll = serverTrustAll;
        serverSecurity.trustList.clear();
        for (size_t i = 0; i < serverTrustList.size(); i++)
            serverSecurity.trustList.push_back(serverTrustList[i]);

        OpcUaClientSecurityConfig clientSecurity = TestClient::CreateSecurityConfig();
        clientSecurity.trustAll = clientTrustAll;
        clientSecurity.trustList.clear();
        for (size_t i = 0; i < clientTrustList.size(); i++)
            clientSecurity.trustList.push_back(clientTrustList[i]);

        TestServer testServer;
        testServer.setSecurityConfig(&serverSecurity);
        testServer.start();

        TestClient testClient;
        testClient.setTimeout(1000);
        testClient.setSecurityConfig(&clientSecurity);
        testClient.connect();
        bool connected = testClient.isConnected();
        testClient.disconnect();
        testServer.stop();

        return connected;
    };

    bool connected;

    connected = tryConnect(false, {}, false, {});
    ASSERT_FALSE(connected);
    connected = tryConnect(false, {clientCert.getValue()}, false, {});
    ASSERT_FALSE(connected);
    connected = tryConnect(false, {}, false, {serverCert.getValue()});
    ASSERT_FALSE(connected);
    connected = tryConnect(false, {clientCert.getValue()}, false, {serverCert.getValue()});
    ASSERT_TRUE(connected);

    connected = tryConnect(true, {}, false, {});
    ASSERT_FALSE(connected);
    connected = tryConnect(true, {clientCert.getValue()}, false, {});
    ASSERT_FALSE(connected);
    connected = tryConnect(true, {}, false, {serverCert.getValue()});
    ASSERT_TRUE(connected);
    connected = tryConnect(true, {clientCert.getValue()}, false, {serverCert.getValue()});
    ASSERT_TRUE(connected);

    connected = tryConnect(false, {}, true, {});
    ASSERT_FALSE(connected);
    connected = tryConnect(false, {clientCert.getValue()}, true, {});
    ASSERT_TRUE(connected);
    connected = tryConnect(false, {}, true, {serverCert.getValue()});
    ASSERT_FALSE(connected);
    connected = tryConnect(false, {clientCert.getValue()}, true, {serverCert.getValue()});
    ASSERT_TRUE(connected);

    connected = tryConnect(true, {}, true, {});
    ASSERT_TRUE(connected);
    connected = tryConnect(true, {clientCert.getValue()}, true, {});
    ASSERT_TRUE(connected);
    connected = tryConnect(true, {}, true, {serverCert.getValue()});
    ASSERT_TRUE(connected);
    connected = tryConnect(true, {clientCert.getValue()}, true, {serverCert.getValue()});
    ASSERT_TRUE(connected);
}

#endif  // OPCUA_ENABLE_ENCRYPTION

END_NAMESPACE_OPENDAQ_OPCUA
