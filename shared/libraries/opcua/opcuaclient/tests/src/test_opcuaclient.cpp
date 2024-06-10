#include <testutils/testutils.h>
#include <future>
#include "opcuaclient/opcuaclient.h"
#include "opcuaservertesthelper.h"
#include "opcuashared/opcua.h"
#include "opcuashared/opcuacommon.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

#define ASSERT_EQ_STATUS(status, expectedStatus) ASSERT_EQ(status, (UA_StatusCode) expectedStatus)

using OpcUaClientTest = BaseClientTest;

static void lockThread(OpcUaClient& client, bool& locked)
{
    locked = client.getLock().try_lock();
    if (locked)
        client.getLock().unlock();
}

static void TestLock(OpcUaClient& client, bool expectedResult)
{
    bool locked;
    std::thread t1(lockThread, std::ref(client), std::ref(locked));
    t1.join();
    ASSERT_EQ(locked, expectedResult);
}

TEST_F(OpcUaClientTest, Lock)
{
    OpcUaClient client(getServerUrl());

    TestLock(client, true);
    {
        auto lockedClient = client.getLockedUaClient();
        TestLock(client, false);
        ASSERT_NE(lockedClient.operator UA_Client*(), nullptr);
        {
            auto lockedClient1 = std::move(lockedClient);
            ASSERT_EQ(lockedClient.operator UA_Client*(), nullptr);
            ASSERT_NE(lockedClient1.operator UA_Client*(), nullptr);

            TestLock(client, false);
        }
        TestLock(client, true);
    }
    TestLock(client, true);
}

TEST_F(OpcUaClientTest, Connect)
{
    OpcUaClient client(getServerUrl());

    client.connect();
    ASSERT_TRUE(client.isConnected());
    client.disconnect();
    ASSERT_FALSE(client.isConnected());
}

TEST_F_OPTIONAL(OpcUaClientTest, FirstConnectFails)
{
    testHelper.stop();

    OpcUaClient client(getServerUrl());

    client.setTimeout(500);

    ASSERT_THROW(client.connect(), OpcUaException);
    ASSERT_FALSE(client.isConnected());

    testHelper.startServer();

    client.disconnect();
    ASSERT_THROW(client.connect(), OpcUaException);
    ASSERT_TRUE(client.isConnected());

    client.disconnect();
    ASSERT_FALSE(client.isConnected());
}

TEST_F(OpcUaClientTest, Timeout)
{
    OpcUaClient client(getServerUrl());

    client.setTimeout(1234u);
    ASSERT_EQ(client.getTimeout(), 1234u);

    client.connect();

    client.setTimeout(1235u);
    ASSERT_EQ(client.getTimeout(), 1235u);

    client.disconnect();

    client.setTimeout(1236u);
    ASSERT_EQ(client.getTimeout(), 1236u);
}

TEST_F(OpcUaClientTest, CheckConnectedStatus)
{
    OpcUaClient client(getServerUrl());

    client.connect();
    ASSERT_TRUE(client.isConnected());
    client.clear();
    ASSERT_FALSE(client.isConnected());

    client.connect();
    client.disconnect();
    ASSERT_FALSE(client.isConnected());
}

TEST_F_OPTIONAL(OpcUaClientTest, ConnectTimeout)
{
    testHelper.stop();
    testHelper.setSessionTimeout(3000);
    testHelper.startServer();

    OpcUaClient client(getServerUrl());

    ASSERT_THROW(client.connect(), OpcUaException);
    ASSERT_TRUE(client.isConnected());

    auto uaNode = OpcUaNodeId(1, ".d");
    ASSERT_TRUE(client.nodeExists(uaNode));

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(70s);

    ASSERT_THROW(client.nodeExists(uaNode), OpcUaException);

    client.disconnect();
    ASSERT_THROW(client.connect(), OpcUaException);
    ASSERT_TRUE(client.isConnected());

    ASSERT_TRUE(client.nodeExists(uaNode));

    client.disconnect();
    ASSERT_FALSE(client.isConnected());
}

TEST_F(OpcUaClientTest, ScheduleTimerTask)
{
    auto client = prepareAndConnectClient();

    ASSERT_FALSE(client->timerTaskExists(1234));

    std::promise<void> promise;
    bool executed = false;
    auto ident1 = client->scheduleTimerTask(1, [&promise, &executed](const OpcUaClient& client, TimerTaskControl& control) {
        if (!executed)
            promise.set_value();
        executed = true;
    });

    ASSERT_TRUE(client->timerTaskExists(ident1));

    IterateAndWaitForPromise(*client, promise.get_future());  // wait until task is executed at least once

    ASSERT_TRUE(client->timerTaskExists(ident1));

    client->removeTimerTask(ident1);

    ASSERT_FALSE(client->timerTaskExists(ident1));
}

TEST_F(OpcUaClientTest, RemoveCurrentTimerTask)
{
    auto client = prepareAndConnectClient();

    std::promise<void> promise;
    auto ident1 = client->scheduleTimerTask(1, [&promise](OpcUaClient& client, TimerTaskControl& control) {
        control.terminateTimerTask();
        promise.set_value();
    });

    IterateAndWaitForPromise(*client, promise.get_future());  // wait until task is executed at least once

    ASSERT_FALSE(client->timerTaskExists(ident1));
}

TEST_F(OpcUaClientTest, NodeExist)
{
    auto client = prepareAndConnectClient();

    OpcUaNodeId uaNode(1, "hello.dewesoft");
    ASSERT_TRUE(client->nodeExists(uaNode));

    uaNode = OpcUaNodeId(1, "f1");
    ASSERT_TRUE(client->nodeExists(uaNode));

    uaNode = OpcUaNodeId(1, "unknown");
    ASSERT_FALSE(client->nodeExists(uaNode));
}

TEST_F(OpcUaClientTest, CallMethod)
{
    auto client = prepareAndConnectClient();

    OpcUaVariant inputArg("Test");

    auto callMethodResult = client->callMethod(
        OpcUaCallMethodRequest(OpcUaNodeId(1, "hello.dewesoft"), OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER), 1, inputArg.get()));

    ASSERT_EQ(callMethodResult->outputArgumentsSize, 1u);
    ASSERT_EQ(OpcUaVariant(callMethodResult->outputArguments[0]).toString(), "Hello! (R:Test)");
}

TEST_F(OpcUaClientTest, CallMethodsWithCallback)
{
    auto client = prepareAndConnectClient();

    std::vector<OpcUaCallMethodRequestWithCallback> callRequests;
    std::string outputArgs;

    OpcUaVariant inputArg("Test");

    callRequests.push_back(OpcUaCallMethodRequestWithCallback(
        OpcUaNodeId(1, "hello.dewesoft"),
        OpcUaNodeId(UA_NS0ID_OBJECTSFOLDER),
        [&](const OpcUaCallMethodResult& callMethodResult) {
            if (!callMethodResult.isStatusOK())
                throw std::runtime_error("call failed with status " + std::to_string(callMethodResult.getStatusCode()));
            else if (!(callMethodResult.getOutputArgumentsSize() == 1 && callMethodResult.getOutputArgument(0).isString()))
                throw std::runtime_error("First argument should be string");
            else
                outputArgs = callMethodResult.getOutputArgument(0).toString();
        },
        1,
        inputArg.get()));

    ASSERT_NO_THROW(client->callMethods(callRequests));
    ASSERT_EQ(outputArgs, "Hello! (R:Test)");
}

END_NAMESPACE_OPENDAQ_OPCUA
