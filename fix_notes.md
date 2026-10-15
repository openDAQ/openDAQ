# Stop the scheduler an instance created when the instance is destroyed

## Bug

Nothing in openDAQ stops the scheduler, so it is destroyed when the last reference to the context drops. Queued work holds references to components, so when an application releases its instance with work in flight, that last reference is often released by one of the scheduler's own workers. The worker cannot destroy its own executor and has to hand it to another thread (#1299). Twelve test sites work around this by calling `scheduler.stop()` by hand before releasing their objects.

## Fix

The instance stops the scheduler it created itself, on the thread that destroys it, after the root device is removed. Queued work drains there while the instance still holds the context, so the context and scheduler are destroyed on that thread, not on a worker. A scheduler supplied through `InstanceBuilder::setScheduler` or `setContext` is left to its owner.

```diff
     const auto builderPtr = InstanceBuilderPtr::Borrow(instanceBuilder);
+    ownsScheduler = !builderPtr.getContext().assigned() && !builderPtr.getScheduler().assigned();
```

```diff
     rootDevice.remove();
     rootDevice.release();
+
+    // Drain and join the scheduler here, so its last reference is not released on one of its own workers
+    if (ownsScheduler)
+        context.getScheduler().stop();
```

Behaviour changes:

- Destroying an instance waits for its queued work. Backlogs of 4.8-17 s were measured in the wav writer test (#1299 analysis).
- Objects that outlive the instance get `OPENDAQ_ERR_SCHEDULER_STOPPED` when they schedule work. The input port ignores that code but leaves the error info set (`input_port_impl.h:396-398`), which `DaqMemCheckListener` reports as an unresolved error on a test thread. [NEEDS-DECISION] Clear it there.
- An instance destroyed on one of its own workers still goes through the hand-off.

## Tests

In `core/opendaq/opendaq/tests/test_instance.cpp`:

- `StopsSchedulerItCreated`: after the instance is gone, `scheduleWork` on its scheduler returns `OPENDAQ_ERR_SCHEDULER_STOPPED`.
- `KeepsSchedulerSuppliedThroughBuilder`: a scheduler passed in through the builder still accepts work after the instance is gone.

The original failure, the last context reference released on a worker at teardown, reproduces in `test_audio_device_module` (`WavWriterTest`) on the gcc-7 32-bit lane with `MALLOC_PERTURB_=165`, without its manual `stop()` (#1299 analysis). Not built or run yet.
