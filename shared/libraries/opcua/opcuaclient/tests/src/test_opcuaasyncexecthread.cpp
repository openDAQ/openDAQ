#include "gtest/gtest.h"

#include "opcuaclient/opcuaasyncexecthread.h"
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaAsyncExecThreadTest = testing::Test;

TEST_F(OpcUaAsyncExecThreadTest, Create)
{
    OpcUaAsyncExecThread asyncExecThread;
}

TEST_F(OpcUaAsyncExecThreadTest, CreateAndRun)
{
    bool executed = false;
    OpcUaAsyncExecThread asyncExecThread = std::thread([&executed]() { executed = true; });
}

TEST_F(OpcUaAsyncExecThreadTest, WaitOnReassign)
{
    bool executed = false;
    OpcUaAsyncExecThread asyncExecThread = std::thread([&executed]() {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);
        executed = true;
    });
    asyncExecThread.reset();
    ASSERT_TRUE(executed);
}

TEST_F(OpcUaAsyncExecThreadTest, WaitOnDestructor)
{
    bool executed = false;
    {
        OpcUaAsyncExecThread asyncExecThread = std::thread([&executed]() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(50ms);
            executed = true;
        });
    }
    ASSERT_TRUE(executed);
}

TEST_F(OpcUaAsyncExecThreadTest, TwoThreads)
{
    bool executed1 = false;
    bool executed2 = false;

    OpcUaAsyncExecThread asyncExecThread = std::thread([&executed1]() {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);
        executed1 = true;
    });

    asyncExecThread = std::thread([&executed2]() { executed2 = true; });
    ASSERT_TRUE(executed1);

    asyncExecThread.reset();
    ASSERT_TRUE(executed2);
}

END_NAMESPACE_OPENDAQ_OPCUA
