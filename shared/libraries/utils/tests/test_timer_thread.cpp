#include <gtest/gtest.h>
#include <math.h>
#include <functional>
#include <chrono>
#include <thread>
#include <iostream>
#include <opendaq/utils/timer_thread.h>

using namespace daq::utils;

using TimerThreadTest = testing::Test;

class TestCallbackClass
{
public:
    int callbackCalls;

    TestCallbackClass() : callbackCalls{0} {};

    void callback()
    {
        callbackCalls++;
    }
};

TEST_F(TimerThreadTest, CreateTimer)
{
    TestCallbackClass callbackFunciton;
    TimerThread timerThread{1000, [&callbackFunciton] { callbackFunciton.callback(); }};
}

TEST_F(TimerThreadTest, CreateAndStart)
{
    TestCallbackClass callbackFunciton;
    TimerThread timerThread{1000, [&callbackFunciton] { callbackFunciton.callback(); }};
    timerThread.start();
}

TEST_F(TimerThreadTest, ExecuteTimer)
{
    TestCallbackClass callbackFunciton;
    TimerThread timerThread{1, [&callbackFunciton] { callbackFunciton.callback(); }};
    ASSERT_EQ(timerThread.getNoOfCallbacks(), callbackFunciton.callbackCalls);
    ASSERT_EQ(callbackFunciton.callbackCalls, 0);
}

TEST_F(TimerThreadTest, TerminateTimerTest)
{
    TimerThread timerThread{1000, nullptr};
    timerThread.start();
    timerThread.stop();
}

TEST_F(TimerThreadTest, ExecuteDelay)
{
    TestCallbackClass callbackFunciton;
    TimerThread timerThread{10000, [&callbackFunciton] { callbackFunciton.callback(); }};
    timerThread.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    timerThread.stop();
    ASSERT_EQ(timerThread.getNoOfCallbacks(), 0);

    callbackFunciton.callbackCalls = 0;
    timerThread.setDelayMs(0);
    timerThread.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    timerThread.stop();
    ASSERT_EQ(timerThread.getNoOfCallbacks(), callbackFunciton.callbackCalls);
    ASSERT_EQ(timerThread.getNoOfCallbacks(), 1);
}
