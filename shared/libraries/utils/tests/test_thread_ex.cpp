#include <testutils/testutils.h>
#include <functional>
#include <chrono>
#include <thread>
#include <opendaq/utils/thread_ex.h>

#include <math.h>

using namespace daq::utils;

using ThreadExTest = testing::Test;

void waitMs(long delayInMs)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(delayInMs));
}

class MyThreadEx : public ThreadEx
{
protected:
    void execute() override
    {
        while (!terminated)
            ;
    }
};

using MyThreadExPtr = std::shared_ptr<MyThreadEx>;

class ThreadExWithWait : public MyThreadEx
{
protected:
    void execute() override
    {
        while (!terminated)
            waitMs(10);
    }
};

class ThreadEngine
{
public:
    virtual ~ThreadEngine() = default;

    virtual void generate(int threadCount)
    {
        for (int i = 0; i < threadCount; i++)
            threads.push_back(std::make_shared<ThreadExWithWait>());
    }

    void start()
    {
        for (auto& thread : threads)
            thread->start();
    }

    void stop()
    {
        for (auto& thread : threads)
        {
            thread->stop();
        }
    }

    bool isFinished()
    {
        for (auto& thread : threads)
        {
            if (!thread->getFinished())
                return false;
        }

        return true;
    }

protected:
    std::vector<MyThreadExPtr> threads;
};

class SetThreadNameThreadEx : public ThreadEx
{
protected:
    void execute() override
    {
        setThreadName("Test");
        while (!terminated)
            ;
    }
};

TEST_F(ThreadExTest, CreateThreadTest)
{
    {
        MyThreadEx thread;
        ASSERT_FALSE(thread.getStarted());
        thread.terminate();
        thread.waitFor();
        ASSERT_FALSE(thread.getStarted());
        ASSERT_FALSE(thread.getFinished());
    }
    {
        MyThreadEx thread;
        ASSERT_FALSE(thread.getStarted());
        thread.start();
        ASSERT_TRUE(thread.getStarted());
        thread.terminate();
        thread.waitFor();
        ASSERT_FALSE(thread.getStarted());
        ASSERT_TRUE(thread.getFinished());
    }
    {
        MyThreadEx thread;
        ASSERT_FALSE(thread.getStarted());
        thread.start();
        ASSERT_TRUE(thread.getStarted());
        thread.stop();
        ASSERT_FALSE(thread.getStarted());
    }
}

TEST_F(ThreadExTest, StartThreadTwoTimes)
{
    MyThreadEx thread;
    thread.start();
    ASSERT_ANY_THROW(thread.start(););
    thread.stop();
}

TEST_F(ThreadExTest, ThreadSetName)
{
    SetThreadNameThreadEx thread;
    thread.start();
    thread.stop();
}

TEST_F(ThreadExTest, MultipleThreadsJoin)
{
    ThreadEngine threadEngine;

#if DS_ENABLE_OPTIONAL_TESTS
    threadEngine.generate(200);
#else
    threadEngine.generate(5);
#endif

    threadEngine.start();

    waitMs(5000);

    threadEngine.stop();

    ASSERT_TRUE(threadEngine.isFinished());
}
