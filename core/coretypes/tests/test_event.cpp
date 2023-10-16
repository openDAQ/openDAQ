#include <gtest/gtest.h>
#include <coretypes/event_args_ptr.h>
#include "event_test.h"
#include <coretypes/delegate.hpp>

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

TEST_F(EventTest, EventSubscriptionMuteFreeFunction)
{
    hasEvent->onEvent += [](BaseObjectPtr& /*prop*/, EventArgsPtr<>& /*args*/)
    {
        // ignore
    };
    hasEvent->onEvent += freeF2;
    hasEvent->onEvent += freeF;

    hasEvent->onEvent |= freeF;  // mute
    hasEvent->triggerEvent();

    ASSERT_EQ(hasEvent->callCount, 0);

    hasEvent->onEvent &= freeF;  // unmute
    hasEvent->triggerEvent();

    ASSERT_EQ(hasEvent->callCount, 1);
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
