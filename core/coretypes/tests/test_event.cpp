#include <gtest/gtest.h>
#include <coretypes/event_args_ptr.h>
#include "event_test.h"
#include <coretypes/delegate.hpp>
#include <atomic>
#include <chrono>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

using namespace daq;

using EventDelegate = delegate<void(BaseObjectPtr&, EventArgsPtr<>&)>;

class EventTest : public testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

public:
    EventTest()
    {
        createObject<IBaseObject, HasEvent>(&ptr);

        hasEvent = dynamic_cast<HasEvent*>(ptr.getObject());
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void onEvent(BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) const
    {
        FAIL() << "Should never be called!";
    }

    // ReSharper disable once CppMemberFunctionMayBeStatic
    void onEvent2(BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) const
    {
    }

    template <typename TReturn, typename TInstance, typename... TArgs>
    auto event(TReturn (TInstance::*func)(TArgs...))
    {
        return delegate<TReturn(TArgs...)>(static_cast<TInstance*>(this), func);
    }

    template <typename TReturn, typename TInstance, typename... TArgs>
    auto event(TReturn (TInstance::*func)(TArgs...) const)
    {
        return delegate<TReturn(TArgs...)>(static_cast<TInstance* const>(this), func);
    }

    HasEvent* hasEvent;
    ObjectPtr<IBaseObject> ptr;
};

class MemberTest
{
public:
    using EventHandler = std::function<void(BaseObjectPtr&, EventArgsPtr<>&)>;

    MemberTest() = default;

    explicit MemberTest(EventHandler&& handler)
        : func(handler)
    {
    }

    void setHandler(EventHandler&& handler)
    {
        func = handler;
    }

    void trigger(BaseObjectPtr& sender, EventArgsPtr<>& args) const
    {
        func(sender, args);
    }

    void trigger2(BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) const
    {
        // func(sender, args);
    }

    void trigger3(BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) const
    {
        // func(sender, args);
    }

private:
    EventHandler func;
};

static void eventHandler(BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
{
}

static void eventHandlerThrows(BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
{
    ASSERT_FALSE(true) << "Should never be called!";
}

static void freeF(BaseObjectPtr& prop, EventArgsPtr<>& /*args*/)
{
    const auto obj = dynamic_cast<HasEvent*>(prop.getObject());
    obj->callCount++;

    // mute to prevent recursive call
    obj->onEvent |= freeF;
    {
        obj->triggerEvent();
    }
    obj->onEvent &= freeF;
}

static void freeF2(BaseObjectPtr& /*prop*/, EventArgsPtr<>& /*args*/)
{
    static int i = 0;

    printf("Called FreeF2 (%d).\n", ++i);
}

static void freeF3(BaseObjectPtr& /*prop*/, EventArgsPtr<>& /*args*/)
{
    FAIL() << "FreeF3 should never be called.";
}

TEST_F(EventTest, FactoryCreate)
{
    auto event = EventObject<BaseObjectPtr, EventArgsPtr<>>();
}

TEST_F(EventTest, Emitter)
{
    Event<BaseObjectPtr, EventArgsPtr<>> ev = EventObject<BaseObjectPtr, EventArgsPtr<>>();
    auto emitter = EventEmitter<BaseObjectPtr, EventArgsPtr<>>(ev);
}

TEST_F(EventTest, EventSubscriptionCount)
{
    HasEvent propObj;
    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 0u);
}

TEST_F(EventTest, EventSubscriptionLambda)
{
    HasEvent propObj;

    // Easy way to bind a callback, but cannot remove later because you can't refer to it anymore.
    // You can only remove it if stored in a variable or by clearing all handlers.
    hasEvent->onEvent += [](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        // ignore
    };

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 1u);
}
//
TEST_F(EventTest, EventSubscriptionMemberRemove)
{
    auto ev = hasEvent->onEvent;

    hasEvent->onEvent += event(&EventTest::onEvent);
    hasEvent->onEvent -= event(&EventTest::onEvent);

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 0u);
}

TEST_F(EventTest, EventSubscriptionMemberRemove2)
{
    MemberTest test1([](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        printf("Called Test 1.\n");
    });

    MemberTest test2([](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        printf("Called Test 2.\n");
    });

    MemberTest test3([](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        printf("Called Test 3.\n");

        FAIL() << "MemberTest3 should never be called.";
    });

    hasEvent->onEvent += ::event(&test1, &MemberTest::trigger);
    hasEvent->onEvent += ::event(&test2, &MemberTest::trigger);
    hasEvent->onEvent += ::event(&test3, &MemberTest::trigger);

    hasEvent->onEvent -= ::event(&test3, &MemberTest::trigger);

    hasEvent->triggerEvent();

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 2u);
}

TEST_F(EventTest, EventSubscriptionMemberRemove3)
{
    Derived d;

    hasEvent->onEvent += ::event(&d, &Derived::onEvent);
    hasEvent->onEvent += event(&EventTest::onEvent);

    hasEvent->onEvent -= event(&EventTest::onEvent);
    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 1u);

    hasEvent->triggerEvent();
    ASSERT_EQ(d.getCallCount(), 1);
}

TEST_F(EventTest, EventSubscriptionMuteMember)
{
    Derived d;

    hasEvent->onEvent += ::event(&d, &Derived::onEvent);
    //hasEvent->onEvent +=  EventDelegate(d, &Derived::onEvent);

    hasEvent->onEvent |= ::event(&d, &Derived::onEvent);
    //hasEvent->onEvent |= EventDelegate(d, &Derived::onEvent);
    hasEvent->triggerEvent();

    ASSERT_EQ(d.getCallCount(), 0);

    hasEvent->onEvent &= ::event(&d, &Derived::onEvent);
    //hasEvent->onEvent &= EventDelegate(d, &Derived::onEvent);
    hasEvent->triggerEvent();

    ASSERT_EQ(d.getCallCount(), 1);
}

TEST_F(EventTest, EventSubscriptionMuteLambda)
{
    int callCount = 0;
    auto lambda = [&callCount](BaseObjectPtr& /*prop*/, EventArgsPtr<>& /*args*/)
    {
        callCount++;
    };
    hasEvent->onEvent += lambda;

    hasEvent->onEvent |= lambda;  // mute
    hasEvent->triggerEvent();

    ASSERT_EQ(callCount, 0);

    hasEvent->onEvent &= lambda;  // unmute
    hasEvent->triggerEvent();

    ASSERT_EQ(callCount, 1);
}

TEST_F(EventTest, EventSubscriptionMute)
{
    auto onChange = hasEvent->onEvent;

    onChange += eventHandlerThrows;
    onChange += event(&EventTest::onEvent);

    Base b;
    onChange += ::event(&b, &Base::onEvent);
    onChange += [](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        FAIL() << "Should never be called!";
    };

    onChange.mute();

    ASSERT_NO_THROW(hasEvent->triggerEvent());
}

TEST_F(EventTest, EventSubscriptionFreeFunction)
{
    hasEvent->onEvent += eventHandler;

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 1u);
}

TEST_F(EventTest, EventSubscriptionFreeFunctionRemove)
{
    hasEvent->onEvent += eventHandler;
    hasEvent->onEvent -= eventHandler;

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 0u);
}

TEST_F(EventTest, EventSubscriptionGlobalAdd)
{
    Derived d;
    hasEvent->onEvent += ::event(&d, &Derived::onEvent);

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 1u);
}

TEST_F(EventTest, EventSubscriptionDerivedRemove)
{
    Derived d;
    hasEvent->onEvent += ::event(&d, &Derived::onEvent);
    hasEvent->onEvent -= ::event(&d, &Derived::onEvent);

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 0u);
}


TEST_F(EventTest, EventSubscriptionLambdaRemove2)
{
    int callCount = 0;
    hasEvent->onEvent += [&callCount](BaseObjectPtr& /*ptr*/, EventArgsPtr<>& /*args*/)
    {
        callCount++;
    };

    auto lambda = [](BaseObjectPtr& /*ptr*/, EventArgsPtr<>& /*args*/)
    {
        FAIL() << "Should not be called!";
    };
    hasEvent->onEvent += lambda;

    hasEvent->onEvent -= lambda;
    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 1u);

    hasEvent->triggerEvent();
    ASSERT_EQ(callCount, 1);
}

TEST_F(EventTest, EventSubscriptionFreeRemove2)
{
    hasEvent->onEvent += freeF2;
    hasEvent->onEvent += freeF3;

    hasEvent->onEvent -= freeF3;

    hasEvent->triggerEvent();

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 1u);
}

TEST_F(EventTest, RemoveHandlerWaitsForHandlerRunningOnAnotherThread)
{
    std::promise<void> handlerEntered;
    std::promise<void> releaseHandler;
    std::shared_future<void> handlerReleased = releaseHandler.get_future().share();
    std::atomic<bool> handlerFinished{false};

    MemberTest handler([&handlerEntered, handlerReleased, &handlerFinished](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        handlerEntered.set_value();
        handlerReleased.wait();
        handlerFinished = true;
    });
    hasEvent->onEvent += ::event(&handler, &MemberTest::trigger);

    std::thread triggerThread([this] { hasEvent->triggerEvent(); });
    handlerEntered.get_future().wait();

    // The handler is still running on the trigger thread. Unsubscribing must wait for it to finish.
    auto removed = std::async(std::launch::async, [this, &handler] { hasEvent->onEvent -= ::event(&handler, &MemberTest::trigger); });
    const bool removeWaited = removed.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout;

    releaseHandler.set_value();
    removed.get();
    triggerThread.join();

    ASSERT_TRUE(removeWaited) << "removeHandler returned while the handler was still running on another thread";
    ASSERT_TRUE(handlerFinished);
    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 0u);
}

TEST_F(EventTest, ClearWaitsForHandlerRunningOnAnotherThread)
{
    std::promise<void> handlerEntered;
    std::promise<void> releaseHandler;
    std::shared_future<void> handlerReleased = releaseHandler.get_future().share();
    std::atomic<bool> handlerFinished{false};

    MemberTest handler([&handlerEntered, handlerReleased, &handlerFinished](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        handlerEntered.set_value();
        handlerReleased.wait();
        handlerFinished = true;
    });
    hasEvent->onEvent += ::event(&handler, &MemberTest::trigger);

    std::thread triggerThread([this] { hasEvent->triggerEvent(); });
    handlerEntered.get_future().wait();

    auto cleared = std::async(std::launch::async, [this] { hasEvent->onEvent = nullptr; });
    const bool clearWaited = cleared.wait_for(std::chrono::milliseconds(200)) == std::future_status::timeout;

    releaseHandler.set_value();
    cleared.get();
    triggerThread.join();

    ASSERT_TRUE(clearWaited) << "clear returned while a handler was still running on another thread";
    ASSERT_TRUE(handlerFinished);
}

TEST_F(EventTest, HandlerRunsWithEventUnlocked)
{
    std::promise<void> handlerEntered;
    std::promise<void> releaseHandler;
    std::shared_future<void> handlerReleased = releaseHandler.get_future().share();
    std::atomic<bool> blocked{false};

    // Only the first call blocks, so the trigger from the other thread below doesn't.
    MemberTest blocking([&handlerEntered, handlerReleased, &blocked](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        if (!blocked.exchange(true))
        {
            handlerEntered.set_value();
            handlerReleased.wait();
        }
    });
    hasEvent->onEvent += ::event(&blocking, &MemberTest::trigger);

    std::thread triggerThread([this] { hasEvent->triggerEvent(); });
    handlerEntered.get_future().wait();

    // While that handler runs, another thread can subscribe to and trigger the same event.
    std::atomic<int> otherCalls{0};
    MemberTest other([&otherCalls](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) { ++otherCalls; });
    auto done = std::async(std::launch::async, [this, &other]
    {
        hasEvent->onEvent += ::event(&other, &MemberTest::trigger);
        hasEvent->triggerEvent();
    });
    const bool completedWhileHandlerRuns = done.wait_for(std::chrono::seconds(5)) == std::future_status::ready;

    releaseHandler.set_value();
    done.get();
    triggerThread.join();

    ASSERT_TRUE(completedWhileHandlerRuns) << "subscribing to or triggering the event blocked while a handler was running";
    ASSERT_EQ(otherCalls, 1);
}

TEST_F(EventTest, HandlerRemovedOnAnotherThreadBeforeItsTurnIsNotCalled)
{
    std::promise<void> handlerEntered;
    std::promise<void> releaseHandler;
    std::shared_future<void> handlerReleased = releaseHandler.get_future().share();

    MemberTest first([&handlerEntered, handlerReleased](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        handlerEntered.set_value();
        handlerReleased.wait();
    });
    std::atomic<int> secondCalls{0};
    MemberTest second([&secondCalls](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) { ++secondCalls; });
    hasEvent->onEvent += ::event(&first, &MemberTest::trigger);
    hasEvent->onEvent += ::event(&second, &MemberTest::trigger);

    std::thread triggerThread([this] { hasEvent->triggerEvent(); });
    handlerEntered.get_future().wait();

    // The second handler hasn't started yet: removing it doesn't wait, and the running trigger must skip it.
    auto removed = std::async(std::launch::async, [this, &second] { hasEvent->onEvent -= ::event(&second, &MemberTest::trigger); });
    const bool removeReturned = removed.wait_for(std::chrono::seconds(5)) == std::future_status::ready;

    releaseHandler.set_value();
    removed.get();
    triggerThread.join();

    ASSERT_TRUE(removeReturned) << "removing a handler that wasn't running blocked";
    ASSERT_EQ(secondCalls, 0);
}

TEST_F(EventTest, HandlerCanRemoveItself)
{
    int calls = 0;
    MemberTest handler;
    handler.setHandler([this, &calls, &handler](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        ++calls;
        hasEvent->onEvent -= ::event(&handler, &MemberTest::trigger);
    });
    hasEvent->onEvent += ::event(&handler, &MemberTest::trigger);

    hasEvent->triggerEvent();
    hasEvent->triggerEvent();

    ASSERT_EQ(calls, 1);
    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 0u);
}

namespace
{
    thread_local bool isLockTakingThread = false;
}

TEST_F(EventTest, HandlerCanTakeLockHeldByThreadTriggeringSameEvent)
{
    // Models a lock cycle between an object lock and the event. This thread holds the object lock and triggers the event,
    // while another thread is inside a handler of the same event and needs the object lock.
    std::timed_mutex objectLock;
    std::promise<void> handlerEntered;
    std::promise<void> triggeringWithLock;
    std::shared_future<void> triggeringWithLockFuture = triggeringWithLock.get_future().share();
    std::atomic<bool> gotObjectLock{false};

    MemberTest handler([&](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        if (!isLockTakingThread)
            return;

        handlerEntered.set_value();
        triggeringWithLockFuture.wait();
        std::unique_lock<std::timed_mutex> lock(objectLock, std::defer_lock);
        gotObjectLock = lock.try_lock_for(std::chrono::seconds(5));
    });
    hasEvent->onEvent += ::event(&handler, &MemberTest::trigger);

    std::thread lockTakingThread([this]
    {
        isLockTakingThread = true;
        hasEvent->triggerEvent();
    });
    handlerEntered.get_future().wait();

    {
        std::lock_guard<std::timed_mutex> lock(objectLock);
        triggeringWithLock.set_value();
        hasEvent->triggerEvent();
    }
    lockTakingThread.join();

    ASSERT_TRUE(gotObjectLock) << "triggering the event while holding the object lock waited for a handler that needs it";
}

TEST_F(EventTest, RemovedHandlerIsNeverCalledAfterRemoveHandlerReturns)
{
    std::atomic<bool> removeReturned{false};
    std::atomic<bool> calledAfterRemove{false};
    std::atomic<bool> stop{false};

    MemberTest handler([&removeReturned, &calledAfterRemove](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        if (removeReturned)
            calledAfterRemove = true;
    });

    std::vector<std::thread> triggerThreads;
    for (int t = 0; t < 2; ++t)
        triggerThreads.emplace_back([this, &stop] { while (!stop) hasEvent->triggerEvent(); });

    for (int i = 0; i < 2000 && !calledAfterRemove; ++i)
    {
        removeReturned = false;
        hasEvent->onEvent += ::event(&handler, &MemberTest::trigger);
        std::this_thread::yield();
        hasEvent->onEvent -= ::event(&handler, &MemberTest::trigger);
        removeReturned = true;
    }

    stop = true;
    for (auto& thread : triggerThreads)
        thread.join();

    ASSERT_FALSE(calledAfterRemove) << "a handler call started after removeHandler returned";
}

TEST_F(EventTest, ConcurrentSubscribeUnsubscribeAndTrigger)
{
    std::atomic<bool> stop{false};
    std::atomic<long> steadyCalls{0};
    MemberTest steady([&steadyCalls](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) { ++steadyCalls; });
    hasEvent->onEvent += ::event(&steady, &MemberTest::trigger);

    std::vector<std::thread> triggerThreads;
    for (int t = 0; t < 4; ++t)
        triggerThreads.emplace_back([this, &stop] { while (!stop) hasEvent->triggerEvent(); });

    // Each subscriber's handler is destroyed right after it is unsubscribed, so a call running after that would use a destroyed object.
    std::vector<std::thread> subscriberThreads;
    for (int t = 0; t < 2; ++t)
    {
        subscriberThreads.emplace_back([this]
        {
            for (int i = 0; i < 1000; ++i)
            {
                MemberTest transient([](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) {});
                hasEvent->onEvent += ::event(&transient, &MemberTest::trigger);
                hasEvent->onEvent -= ::event(&transient, &MemberTest::trigger);
            }
        });
    }

    for (auto& thread : subscriberThreads)
        thread.join();
    stop = true;
    for (auto& thread : triggerThreads)
        thread.join();

    ASSERT_EQ(hasEvent->onEvent.getListenerCount(), 1u);
    ASSERT_GT(steadyCalls.load(), 0);
}

TEST_F(EventTest, MutingDuringTriggerAppliesFromTheNextTrigger)
{
    int secondCalls = 0;
    MemberTest second([&secondCalls](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/) { ++secondCalls; });
    MemberTest first([this, &second](BaseObjectPtr& /*sender*/, EventArgsPtr<>& /*args*/)
    {
        hasEvent->onEvent.muteListener(::event(&second, &MemberTest::trigger));
    });
    hasEvent->onEvent += ::event(&first, &MemberTest::trigger);
    hasEvent->onEvent += ::event(&second, &MemberTest::trigger);

    // A trigger uses the muted state it started with, so the second handler still runs this time...
    hasEvent->triggerEvent();
    ASSERT_EQ(secondCalls, 1);

    // ...and is skipped from the next trigger on.
    hasEvent->triggerEvent();
    ASSERT_EQ(secondCalls, 1);
}

TEST_F(EventTest, DelegateMemberEquality)
{
    Derived obj;

    auto d1 = ::event(&obj, &Derived::onEvent);
    auto d2 = ::event(&obj, &Derived::onEvent);

    ASSERT_EQ(d1, d2);
}

TEST_F(EventTest, DelegateMemberDifferenceFunc)
{
    Derived obj;

    auto d1 = ::event(&obj, &Derived::onEvent);
    auto d2 = ::event(&obj, &Derived::onEvent2);

    ASSERT_FALSE(d1 == d2);
}

TEST_F(EventTest, DelegateMemberDifferenceObj)
{
    Derived obj;
    Derived obj2;

    auto d1 = ::event(&obj, &Derived::onEvent);
    auto d2 = ::event(&obj2, &Derived::onEvent);

    ASSERT_FALSE(d1 == d2);
}

TEST_F(EventTest, DelegateMemberThisEquality)
{
    auto d1 = event(&EventTest::onEvent);
    auto d2 = event(&EventTest::onEvent);

    ASSERT_EQ(d1, d2);
}

TEST_F(EventTest, DelegateMemberThisDifferenceFun)
{
    auto d1 = event(&EventTest::onEvent);
    auto d2 = event(&EventTest::onEvent2);

    ASSERT_FALSE(d1 == d2);
}

TEST_F(EventTest, DelegateMemberNoWrapperEquality)
{
    Derived d;

    EventDelegate d1(&d, &Derived::onEvent);
    EventDelegate d2(&d, &Derived::onEvent);

    ASSERT_EQ(d1, d2);
}

TEST_F(EventTest, DelegateMemberNoWrapperDifferenceObj)
{
    Derived obj;
    Derived obj2;

    EventDelegate d1(&obj, &Derived::onEvent);
    EventDelegate d2(&obj2, &Derived::onEvent);

    ASSERT_FALSE(d1 == d2);
}

TEST_F(EventTest, DelegateMemberNoWrapperDifferenceFun)
{
    Derived obj;

    EventDelegate d1(&obj, &Derived::onEvent);
    EventDelegate d2(&obj, &Derived::onEvent2);

    ASSERT_FALSE(d1 == d2);
}

TEST_F(EventTest, DelegateLambdaEquality)
{
    auto lambda = [](BaseObjectPtr&, EventArgsPtr<>&)
    {
        // ignore
    };

    EventDelegate d1 = lambda;
    EventDelegate d2 = lambda;

    ASSERT_EQ(d1, d2);
}

TEST_F(EventTest, DelegateLambdaDifference)
{
    auto lambda = [](BaseObjectPtr&, EventArgsPtr<>&)
    {
        // ignore
    };

    auto lambda2 = [](BaseObjectPtr&, EventArgsPtr<>&)
    {
        // ignore
    };

    EventDelegate d1 = lambda;
    EventDelegate d2 = lambda2;

    ASSERT_FALSE(d1 == d2);
}

TEST_F(EventTest, DelegateGlobalEquality)
{
    EventDelegate d1 = eventHandler;
    EventDelegate d2 = eventHandler;

    ASSERT_EQ(d1, d2);
}

TEST_F(EventTest, DelegateGlobalDifference)
{
    EventDelegate d1 = eventHandler;
    EventDelegate d2 = freeF2;

    ASSERT_FALSE(d1 == d2);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IEvent", "daq");

TEST_F(EventTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IEvent::Id);
}

TEST_F(EventTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IEvent>(), "{82774C35-1638-5228-9A72-8DDDFDF10C10}");
}
