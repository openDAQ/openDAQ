# Join the thread that destroys an executor released on its own worker

## Bug

When the last reference to a scheduler is released inside a work callback, `SchedulerImpl::stop()` runs on one of its own workers. Taskflow's `~Executor` waits for all work and joins every worker, so it cannot run there. Since #1299, the executor is moved into a detached `std::thread`. Nobody can wait for it, so:

- `SchedulerTestCommon.LastReferenceReleasedFromWorker` fails intermittently with "Memory leaks detected" on the MSVC debug lane: the test ends while the detached thread is still freeing the executor (run 38698023557, job 116299223833, on PR #1352).
- On Linux, `exit()` can run Taskflow's static node-pool destructor while that thread's workers still run tasks, which is the crash #1299 narrowed but did not close.

## Fix

The hand-off thread is joined instead of detached. One library-owned slot holds the latest hand-off thread; each new one joins its predecessor first, so a single join covers all of them. A `stop()` on a thread that is not a scheduler worker joins the slot before destroying its own executor, and the slot's destructor joins it at library teardown. A `thread_local` flag set in the worker prologue marks threads of any scheduler, which hand off instead of joining, so a callback never waits for a chain that waits for that callback.

```diff
+    if (executor && onSchedulerWorker)
+        executorHandOff.start(std::move(executor));
     else
     {
+        executorHandOff.join();
         executor.reset();
     }
```

```cpp
void start(std::unique_ptr<tf::Executor> executor)
{
    std::lock_guard lock(mutex);
    thread = std::thread([executor = std::move(executor), previous = std::move(thread)]() mutable
    {
        if (previous.joinable())
            previous.join();
        executor.reset();
    });
}
```

Cost: a `stop()` on an application thread waits for executors handed off earlier, including their queued work.

## Tests

`SchedulerTestCommon.LastReferenceReleasedFromWorker` (`core/opendaq/scheduler/tests/test_scheduler.cpp`) reproduces it under load on MSVC debug. It now destroys a second scheduler on the test thread, whose `stop()` joins the hand-off, instead of polling until the work callback is gone:

```diff
-    const auto deadline = std::chrono::steady_clock::now() + 5s;
-    while (!callbackAlive.expired() && std::chrono::steady_clock::now() < deadline)
-        std::this_thread::sleep_for(1ms);
+    Scheduler(logger, 1);
     ASSERT_TRUE(callbackAlive.expired());
```

Not built or run yet. To check: `test_scheduler --gtest_repeat=200` on MSVC debug.
