#include <gtest/gtest.h>
#include <opendaq/utils/function_thread.h>

using namespace daq::utils;

using FunctionThreadTest = testing::Test;

TEST_F(FunctionThreadTest, NoCallbackTest)
{
    FunctionThread thread;
    thread.start();
    thread.waitFor();
}

TEST_F(FunctionThreadTest, WithCallbackTest)
{
    {
        int counter = 0;
        FunctionThread thread([&counter]() { counter++; });
        ASSERT_EQ(counter, 0);
        thread.start();
        thread.waitFor();
        ASSERT_EQ(counter, 1);
    }

    {
        int counter = 0;
        FunctionThread thread;
        thread.setCallback([&counter]() { counter++; });
        ASSERT_EQ(counter, 0);
        thread.start();
        thread.waitFor();
        ASSERT_EQ(counter, 1);
    }
}
