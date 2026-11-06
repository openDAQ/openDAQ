// White-box tests of the MultiReader2 ingest task, which coalesces the ports' notifications into one drain at a time. Its
// source is compiled into this test, as it is not exported from the library.

#include <gtest/gtest.h>
#include <opendaq/multi_reader2_ingest_task.h>
#include <opendaq/opendaq.h>
#include <opendaq/work_factory.h>

#include <array>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace daq;
using namespace daq::multi_reader2;

TEST(MultiReader2IngestTask, FirstNotificationSchedulesAndLaterOnesWaitForTheRun)
{
    IngestTask task;
    EXPECT_TRUE(task.notify());
    EXPECT_FALSE(task.notify());
    EXPECT_FALSE(task.run([] {}));
    EXPECT_TRUE(task.notify());
}

TEST(MultiReader2IngestTask, NotificationDuringTheDrainSchedulesOneMoreRun)
{
    IngestTask task;
    ASSERT_TRUE(task.notify());
    EXPECT_TRUE(task.run(
        [&]
        {
            EXPECT_FALSE(task.notify());
            EXPECT_FALSE(task.notify());
        }));
    EXPECT_FALSE(task.notify());
    EXPECT_FALSE(task.run([] {}));
    EXPECT_TRUE(task.notify());
}

TEST(MultiReader2IngestTask, DrainThatThrowsLeavesTheTaskIdleForTheNextNotification)
{
    IngestTask task;
    ASSERT_TRUE(task.notify());
    EXPECT_THROW(task.run(
                     [&]
                     {
                         EXPECT_FALSE(task.notify());
                         throw std::runtime_error("drain failed");
                     }),
                 std::runtime_error);
    EXPECT_TRUE(task.notify());
}

TEST(MultiReader2IngestTask, RunThatCouldNotBeScheduledIsScheduledOnTheNextNotification)
{
    IngestTask task;
    ASSERT_TRUE(task.notify());
    task.cancel();
    EXPECT_TRUE(task.notify());
}

// Senders mark and notify as the ports do, on the openDAQ scheduler: every value sent is drained, no two drains overlap,
// and the drains are fewer than the notifications
TEST(MultiReader2IngestTask, ConcurrentSendersLoseNoPacketAndDrainOneAtATime)
{
    constexpr int senders = 8;
    constexpr int packets = 20000;
    const auto scheduler = Scheduler(LoggerWithSinks(List<ILoggerSink>()), 4);
    IngestTask task;
    std::array<std::atomic<int>, senders> sent{};
    std::array<std::atomic<bool>, senders> dirty{};
    std::array<int, senders> drained{};
    std::atomic<int> inside{0};
    std::atomic<bool> overlapped{false};
    std::atomic<int> drains{0};

    WorkPtr work;
    work = Work(
        [&]
        {
            const bool again = task.run(
                [&]
                {
                    if (inside.fetch_add(1) != 0)
                        overlapped = true;
                    for (int i = 0; i < senders; ++i)
                        if (dirty[i].exchange(false))
                            drained[i] = sent[i].load();
                    ++drains;
                    inside.fetch_sub(1);
                });
            if (again)
                scheduler.scheduleWork(work);
        });

    std::vector<std::thread> threads;
    for (int s = 0; s < senders; ++s)
    {
        threads.emplace_back(
            [&, s]
            {
                for (int p = 1; p <= packets; ++p)
                {
                    sent[s] = p;
                    dirty[s] = true;
                    if (task.notify())
                        scheduler.scheduleWork(work);
                }
            });
    }
    for (auto& t : threads)
        t.join();
    scheduler.waitAll();

    for (int s = 0; s < senders; ++s)
        EXPECT_EQ(drained[s], packets) << "sender " << s;
    EXPECT_FALSE(overlapped);
    EXPECT_LT(drains.load(), senders * packets);
    EXPECT_TRUE(task.notify());
}

TEST(MultiReader2IngestTask, IdleOnlyWithNoRunScheduledOrRunning)
{
    IngestTask task;
    EXPECT_TRUE(task.idle());
    ASSERT_TRUE(task.notify());
    EXPECT_FALSE(task.idle());
    EXPECT_FALSE(task.run([&] { EXPECT_FALSE(task.idle()); }));
    EXPECT_TRUE(task.idle());
}
