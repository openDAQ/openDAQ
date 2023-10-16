#include <testutils/testutils.h>

#include <opendaq/utils/finally.h>
#include "opcuaclient/opcuatimertaskhelper.h"

#include "opcuaclient/taskprocessor/opcuataskprocessor.h"
#include "opcuaservertesthelper.h"

using namespace daq::utils;
using namespace std::chrono_literals;

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaTimerTaskHelperTest = BaseClientTest;

TEST_F(OpcUaTimerTaskHelperTest, Create)
{
    OpcUaClientPtr client = std::make_unique<OpcUaClient>(getServerUrl());
    client->connect();

    OpcUaTaskProcessor taskProcessor(client);
    taskProcessor.start();

    std::promise<void> promise;
    std::atomic<bool> promiseIsSet = false;
    OpcUaTimerTaskHelper timerTaskHelper(*client, 1, [&promise, &promiseIsSet](OpcUaClient& client) {
        if (!promiseIsSet)
            promise.set_value();
        promiseIsSet = true;
    });

    ASSERT_EQ(timerTaskHelper.getIntervalMs(), 1);
    ASSERT_FALSE(timerTaskHelper.getTerminated());
    ASSERT_FALSE(timerTaskHelper.getStarted());

    timerTaskHelper.start();

    ASSERT_FALSE(timerTaskHelper.getTerminated());
    ASSERT_TRUE(timerTaskHelper.getStarted());

    ASSERT_NE(promise.get_future().wait_for(2s), std::future_status::timeout); // wait until task is executed

    timerTaskHelper.terminate();

    ASSERT_TRUE(timerTaskHelper.getTerminated());
    ASSERT_TRUE(timerTaskHelper.getStarted());

    timerTaskHelper.stop();

    ASSERT_FALSE(timerTaskHelper.getStarted());

    taskProcessor.stop();
}

TEST_F(OpcUaTimerTaskHelperTest, ThrowExceptionInTask)
{
    OpcUaClientPtr client = std::make_unique<OpcUaClient>(getServerUrl());
    client->connect();

    OpcUaTaskProcessor taskProcessor(client);
    taskProcessor.start();

    std::promise<void> promise;
    std::atomic<bool> promiseIsSet = false;
    OpcUaTimerTaskHelper timerTaskHelper(*client, 1, [&promise, &promiseIsSet](OpcUaClient& client) {
        Finally finnaly([&promise, &promiseIsSet]() {
            if (!promiseIsSet)
                promise.set_value();
            promiseIsSet = true;
        });

        throw std::runtime_error("test error");
    });

    ASSERT_EQ(timerTaskHelper.getIntervalMs(), 1);
    ASSERT_FALSE(timerTaskHelper.getTerminated());
    ASSERT_FALSE(timerTaskHelper.getStarted());

    timerTaskHelper.start();

    ASSERT_FALSE(timerTaskHelper.getTerminated());
    ASSERT_TRUE(timerTaskHelper.getStarted());

    ASSERT_NE(promise.get_future().wait_for(2s), std::future_status::timeout); // wait until task is executed

    timerTaskHelper.terminate();

    ASSERT_TRUE(timerTaskHelper.getTerminated());
    ASSERT_TRUE(timerTaskHelper.getStarted());

    timerTaskHelper.stop();

    ASSERT_FALSE(timerTaskHelper.getStarted());
}

END_NAMESPACE_OPENDAQ_OPCUA
