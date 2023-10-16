#include <testutils/testutils.h>

#include <opcuashared/opcuanodeid.h>
#include "opcuaclient/taskprocessor/opcuataskprocessor.h"
#include "opcuaservertesthelper.h"

using namespace std::chrono_literals;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaTaskProcessorTest = BaseClientTest;

static void TestRequest(OpcUaTaskProcessor& taskProcessor)
{
    std::exception_ptr exception;
    taskProcessor.executeTask(
        [&exception](OpcUaClient& client)
        {
            OpcUaNodeId uaNode(1, "hello.dewesoft");
            try
            {
                client.nodeExists(uaNode);
            }
            catch (const OpcUaException&)
            {
                exception = std::current_exception();
            }
        });

    if (exception)
        std::rethrow_exception(exception);
}

TEST_F(OpcUaTaskProcessorTest, Create)
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    ASSERT_EQ(taskProcessor.getClient(), client);

    taskProcessor.start();

    ASSERT_TRUE(taskProcessor.isConnected());

    taskProcessor.stop();
}

TEST_F(OpcUaTaskProcessorTest, CreateAndStopInDestructor)
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    taskProcessor.start();
}

TEST_F(OpcUaTaskProcessorTest, ExecuteTaskAwait)
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    taskProcessor.start();

    bool executed = false;
    std::future<void> future = taskProcessor.executeTaskAwait([&executed](const OpcUaClient& client) { executed = true; });

    future.get();

    ASSERT_TRUE(executed);

    taskProcessor.stop();
}

TEST_F(OpcUaTaskProcessorTest, ExecuteTask)
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    taskProcessor.start();

    bool executed = false;
    taskProcessor.executeTask([&executed](const OpcUaClient& client) { executed = true; });

    ASSERT_TRUE(executed);

    taskProcessor.stop();
}

TEST_F(OpcUaTaskProcessorTest, ExceptionInTask)
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    taskProcessor.start();

    ASSERT_THROW(taskProcessor.executeTask([](const OpcUaClient& client) { throw std::runtime_error("test exception"); }),
                 std::runtime_error);

    std::future<void> future =
        taskProcessor.executeTaskAwait([](const OpcUaClient& client) { throw std::runtime_error("test exception"); });

    ASSERT_THROW(future.get(), std::runtime_error);

    bool executed = false;
    taskProcessor.executeTask([&executed](const OpcUaClient& client) { executed = true; });

    ASSERT_TRUE(executed);

    taskProcessor.stop();
}

TEST_F(OpcUaTaskProcessorTest, AddTaskInsideOfTask)  // test for possible deadlock
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    taskProcessor.start();

    int number = 0;
    taskProcessor.executeTask(
        [&taskProcessor, &number](const OpcUaClient& client)
        {
            taskProcessor.executeTask([&number](const OpcUaClient& client) { number++; });
            number++;
        });

    ASSERT_EQ(number, 2);

    taskProcessor.stop();
}

TEST_F(OpcUaTaskProcessorTest, Disconnect)
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    taskProcessor.start();

    ASSERT_TRUE(taskProcessor.isConnected());

    testHelper.stop();

    ASSERT_THROW(TestRequest(taskProcessor), OpcUaException);

    ASSERT_FALSE(taskProcessor.isConnected());

    taskProcessor.stop();
}

TEST_F(OpcUaTaskProcessorTest, SetTimeout)
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    taskProcessor.start();

    taskProcessor.setConnectionTimeout(1234u);

    uint32_t timeout{};
    taskProcessor.executeTask([&timeout](OpcUaClient& client) { timeout = client.getTimeout(); });

    ASSERT_EQ(timeout, 1234u);

    taskProcessor.stop();
}

TEST_F(OpcUaTaskProcessorTest, AddTaskInsideOfTimerTask)
{
    auto client = prepareAndConnectClient();
    OpcUaTaskProcessor taskProcessor(client);

    taskProcessor.start();

    std::promise<void> promise;
    int number = 0;
    auto ident1 = client->scheduleTimerTask(1,
                                            [&taskProcessor, &promise, &number](OpcUaClient& client, TimerTaskControl& control)
                                            {
                                                control.terminateTimerTask();

                                                taskProcessor.executeTask([&number](const OpcUaClient& client) { number++; });
                                                number++;

                                                promise.set_value();
                                            });

    ASSERT_NE(promise.get_future().wait_for(2s), std::future_status::timeout); // wait until task is executed

    ASSERT_EQ(number, 2);

    ASSERT_FALSE(client->timerTaskExists(ident1));

    taskProcessor.stop();
}

END_NAMESPACE_OPENDAQ_OPCUA
