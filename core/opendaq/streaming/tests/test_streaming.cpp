#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/gmock/streaming.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/input_port_factory.h>
#include "mock/mock_mirrored_signal.h"
#include <opendaq/component_factory.h>

BEGIN_NAMESPACE_OPENDAQ

using namespace testing;

class StreamingTest : public testing::Test
{
public:
    StreamingTest()
        : context(NullContext())
        , streaming(MockStreaming::Strict("MockStreaming", context))
        , streamingPrivate(streaming.ptr.template asPtr<IStreamingPrivate>(true))
    {}

    ContextPtr context;
    MockStreaming::Strict streaming;
    IStreamingPrivate* streamingPrivate;
};

TEST_F(StreamingTest, ConnectionString)
{
    ASSERT_EQ(streaming.ptr.getConnectionString(), "MockStreaming");
}

TEST_F(StreamingTest, InactiveByDefault)
{
    daq::Bool active;
    ASSERT_NO_THROW(active = streaming.ptr.getActive());
    ASSERT_EQ(active, False);

    EXPECT_CALL(streaming.mock(), onSetActive(_)).Times(Exactly(0));
    ASSERT_NO_THROW(streaming.ptr.setActive(False));
    ASSERT_EQ(streaming.ptr->setActive(False), OPENDAQ_IGNORED);
}

TEST_F(StreamingTest, Activate)
{
    EXPECT_CALL(streaming.mock(), onSetActive(true)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(True));
    ASSERT_EQ(streaming.ptr.getActive(), True);
}

TEST_F(StreamingTest, ActivateTwice)
{
    EXPECT_CALL(streaming.mock(), onSetActive(true)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(True));
    ASSERT_EQ(streaming.ptr.getActive(), True);

    EXPECT_CALL(streaming.mock(), onSetActive(_)).Times(Exactly(0));
    ASSERT_NO_THROW(streaming.ptr.setActive(True));
    ASSERT_EQ(streaming.ptr->setActive(True), OPENDAQ_IGNORED);
    ASSERT_EQ(streaming.ptr.getActive(), True);
}

TEST_F(StreamingTest, Deactivate)
{
    EXPECT_CALL(streaming.mock(), onSetActive(true)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(True));

    EXPECT_CALL(streaming.mock(), onSetActive(false)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(False));
    ASSERT_EQ(streaming.ptr.getActive(), False);
}

TEST_F(StreamingTest, SignalAvailableUnavailableSuccess)
{
    ASSERT_NO_THROW(streaming.mock().makeSignalAvailable("Signal"));
    ASSERT_NO_THROW(streaming.mock().makeSignalUnavailable("Signal"));
}

TEST_F(StreamingTest, SignalAvailableUnavailableFail)
{
    ASSERT_THROW(streaming.mock().makeSignalUnavailable("Signal"), NotFoundException);

    ASSERT_NO_THROW(streaming.mock().makeSignalAvailable("Signal"));
    ASSERT_THROW(streaming.mock().makeSignalAvailable("Signal"), DuplicateItemException);

    ASSERT_NO_THROW(streaming.mock().makeSignalUnavailable("Signal"));
    ASSERT_THROW(streaming.mock().makeSignalUnavailable("Signal"), NotFoundException);
}

TEST_F(StreamingTest, StartCompleteReconnection)
{
    // reconnection cannot be completed without being started
    ASSERT_THROW(streaming.mock().triggerReconnectionCompletion(), InvalidStateException);

    ASSERT_NO_THROW(streaming.mock().triggerReconnectionStart());
    // reconnection can be restarted multiple times before completion
    ASSERT_NO_THROW(streaming.mock().triggerReconnectionStart());

    // signal unavailable triggering is not allowed during reconnection
    ASSERT_THROW(streaming.mock().makeSignalUnavailable("Signal"), InvalidStateException);

    ASSERT_NO_THROW(streaming.mock().triggerReconnectionCompletion());
    ASSERT_THROW(streaming.mock().triggerReconnectionCompletion(), InvalidStateException);
}

TEST_F(StreamingTest, TriggerAvailableDuringReconnection)
{
    ASSERT_NO_THROW(streaming.mock().triggerReconnectionStart());

    ASSERT_NO_THROW(streaming.mock().makeSignalAvailable("Signal1"));
    ASSERT_NO_THROW(streaming.mock().makeSignalAvailable("Signal2"));
    ASSERT_NO_THROW(streaming.mock().makeSignalAvailable("Signal3"));

    ASSERT_NO_THROW(streaming.mock().triggerReconnectionCompletion());

    ASSERT_THROW(streaming.mock().makeSignalAvailable("Signal1"), DuplicateItemException);
    ASSERT_THROW(streaming.mock().makeSignalAvailable("Signal2"), DuplicateItemException);
    ASSERT_THROW(streaming.mock().makeSignalAvailable("Signal3"), DuplicateItemException);

    ASSERT_NO_THROW(streaming.mock().makeSignalUnavailable("Signal1"));
    ASSERT_NO_THROW(streaming.mock().makeSignalUnavailable("Signal2"));
    ASSERT_NO_THROW(streaming.mock().makeSignalUnavailable("Signal3"));
}

// 1-st parameter signal available in streaming (true) or not (false)
// 2-nd parameter signal has parent (true) and remote and streaming Ids are different, or not (false)
class SignalAddRemoveTest : public StreamingTest, public WithParamInterface<std::tuple<bool, bool>>
{
public:
    SignalAddRemoveTest()
        : StreamingTest()
        , isSignalAvailable(std::get<0>(GetParam()))
        , parentComponent(std::get<1>(GetParam()) ? Component(context, nullptr, "parentId") : nullptr)
    {
    }

    MirroredSignalConfigPtr createMirroredSignal(const StringPtr& id)
    {
        if (isSignalAvailable)
            streaming.mock().makeSignalAvailable(id);
        auto signal = createWithImplementation<IMirroredSignalConfig, MockMirroredSignalImpl>(context, parentComponent, id);
        signal.asPtr<IMirroredSignalPrivate>().setMirroredDataDescriptor(DataDescriptorBuilder().build());
        return signal;
    }

    bool isSignalAvailable;
    ComponentPtr parentComponent;
};

TEST_P(SignalAddRemoveTest, AddSignalInvalid)
{
    auto signal = Signal(context, nullptr, String("Signal"));

    ASSERT_THROW(streaming.ptr.addSignals({signal}), NoInterfaceException);
}

TEST_P(SignalAddRemoveTest, AddSignal)
{
    auto signal = createMirroredSignal("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

TEST_P(SignalAddRemoveTest, AddSignalTwice)
{
    auto signal = createMirroredSignal("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
    ASSERT_THROW(streaming.ptr.addSignals({signal}), DuplicateItemException);
}

TEST_P(SignalAddRemoveTest, AddSignalsSameId)
{
    auto signal = createMirroredSignal("Signal");

    auto signal2 = createWithImplementation<IMirroredSignalConfig, MockMirroredSignalImpl>(context, parentComponent, "Signal");
    signal2.asPtr<IMirroredSignalPrivate>().setMirroredDataDescriptor(DataDescriptorBuilder().build());

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(1));
    ASSERT_THROW(streaming.ptr.addSignals({signal, signal2}), DuplicateItemException);
}

TEST_P(SignalAddRemoveTest, AddMultipleSignals)
{
    auto signal = createMirroredSignal("Signal");
    auto signal2 = createMirroredSignal("Signal2");

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal, signal2}));
}

TEST_P(SignalAddRemoveTest, RemoveSignalInvalid)
{
    auto signal = Signal(context, nullptr, String("Signal"));

    ASSERT_THROW(streaming.ptr.removeSignals({signal}), NoInterfaceException);
}

TEST_P(SignalAddRemoveTest, RemoveSignalNotFound)
{
    auto signal = createMirroredSignal("Signal");

    ASSERT_THROW(streaming.ptr.removeSignals({signal}), NotFoundException);
}

TEST_P(SignalAddRemoveTest, RemoveSignalValid)
{
    auto signal = createMirroredSignal("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal}));
}

TEST_P(SignalAddRemoveTest, AddSignalAfterRemove)
{
    auto signal = createMirroredSignal("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal}));

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

TEST_P(SignalAddRemoveTest, RemoveMultipleSignals)
{
    auto signal1 = createMirroredSignal("Signal");
    auto signal2 = createMirroredSignal("Signal2");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal1)).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onAddSignal(signal2)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal1, signal2}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal1)).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal2)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal1, signal2}));
}

TEST_P(SignalAddRemoveTest, RemoveAllNoSignals)
{
    ASSERT_NO_THROW(streaming.ptr.removeAllSignals());
}

TEST_P(SignalAddRemoveTest, RemoveAllOneSignal)
{
    auto signal = createMirroredSignal("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeAllSignals());

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

INSTANTIATE_TEST_SUITE_P(
    SignalAddRemove,
    SignalAddRemoveTest,
    testing::Values(
        std::make_tuple(false, false),
        std::make_tuple(true, false),
        std::make_tuple(false, true),
        std::make_tuple(true, true)
    )
);

// test parameter signal has parent (true) and remote and streaming Ids are different, or not (false)
class DirectSubscriptionTest : public StreamingTest, public WithParamInterface<bool>
{
public:
    DirectSubscriptionTest()
        : StreamingTest()
        , parentComponent(GetParam() ? Component(context, nullptr, "parentId") : nullptr)
    {
    }

    MirroredSignalConfigPtr createAndAddSignal(const StringPtr& id)
    {
        auto signal = createWithImplementation<IMirroredSignalConfig, MockMirroredSignalImpl>(context, parentComponent, id);
        signal.asPtr<IMirroredSignalPrivate>().setMirroredDataDescriptor(DataDescriptorBuilder().build());

        streaming.mock().makeSignalAvailable(id);
        EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
        streaming.ptr.addSignals({signal});

        return signal;
    }

    ComponentPtr parentComponent;
};

TEST_P(DirectSubscriptionTest, DirectSubscribeUnsubscribe)
{
    auto signal = createAndAddSignal("Signal");

    SubscriptionEventArgsPtr subscribeEventArgs;
    signal.getOnSubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { subscribeEventArgs = args; };

    SubscriptionEventArgsPtr unsubscribeEventArgs;
    signal.getOnUnsubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { unsubscribeEventArgs = args; };

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streamingPrivate->subscribeSignal(signal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );

    ASSERT_EQ(subscribeEventArgs.getStreamingConnectionString(), "MockStreaming");
    ASSERT_EQ(subscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Subscribed);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streamingPrivate->unsubscribeSignal(signal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );

    ASSERT_EQ(unsubscribeEventArgs.getStreamingConnectionString(), "MockStreaming");
    ASSERT_EQ(unsubscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Unsubscribed);
}

TEST_P(DirectSubscriptionTest, DirectWithDomain)
{
    auto signal = createAndAddSignal("Signal");
    auto domainSignal = createAndAddSignal("DomainSignal");

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streamingPrivate->subscribeSignal(signal.getRemoteId(), domainSignal.getRemoteId()),
        OPENDAQ_SUCCESS
    );

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streamingPrivate->unsubscribeSignal(signal.getRemoteId(), domainSignal.getRemoteId()),
        OPENDAQ_SUCCESS
    );
}

TEST_P(DirectSubscriptionTest, SubscriptionCounter)
{
    auto signal1 = createAndAddSignal("Signal1");
    auto signal2 = createAndAddSignal("Signal2");
    auto domainSignal = createAndAddSignal("DomainSignal");

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal1.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streamingPrivate->subscribeSignal(signal1.getRemoteId(), domainSignal.getRemoteId()),
        OPENDAQ_SUCCESS
    );// signal1 counter = 1; signal2 counter = 0; domainSignal = 1

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal2.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(
        streamingPrivate->subscribeSignal(signal2.getRemoteId(), domainSignal.getRemoteId()),
        OPENDAQ_SUCCESS
    );// signal1 counter = 1; signal2 counter = 1; domainSignal = 2

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(
        streamingPrivate->subscribeSignal(domainSignal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );// signal1 counter = 1; signal2 counter = 1; domainSignal = 3

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal1.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(
        streamingPrivate->unsubscribeSignal(signal1.getRemoteId(), domainSignal.getRemoteId()),
        OPENDAQ_SUCCESS
    );// signal1 counter = 0; signal2 counter = 1; domainSignal = 2

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal2.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(
        streamingPrivate->unsubscribeSignal(signal2.getRemoteId(), domainSignal.getRemoteId()),
        OPENDAQ_SUCCESS
    );// signal1 counter = 0; signal2 counter = 0; domainSignal = 1

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streamingPrivate->unsubscribeSignal(domainSignal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );// signal1 counter = 0; signal2 counter = 0; domainSignal = 0
}

TEST_P(DirectSubscriptionTest, InvalidParameters)
{
    auto signal = createAndAddSignal("Signal");
    auto domainSignal = createAndAddSignal("DomainSignal");

    ASSERT_EQ(streamingPrivate->subscribeSignal(nullptr, domainSignal.getRemoteId()), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(streamingPrivate->subscribeSignal(signal.getRemoteId(), signal.getRemoteId()), OPENDAQ_ERR_INVALIDPARAMETER);
    ASSERT_EQ(streamingPrivate->subscribeSignal("OtherSignalId", nullptr), OPENDAQ_ERR_NOTFOUND);
    ASSERT_EQ(streamingPrivate->subscribeSignal(signal.getRemoteId(), "OtherSignalId"), OPENDAQ_ERR_NOTFOUND);

    ASSERT_EQ(streamingPrivate->unsubscribeSignal(nullptr, domainSignal.getRemoteId()), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(streamingPrivate->unsubscribeSignal(signal.getRemoteId(), signal.getRemoteId()), OPENDAQ_ERR_INVALIDPARAMETER);
    ASSERT_EQ(streamingPrivate->unsubscribeSignal("OtherSignalId", nullptr), OPENDAQ_ERR_NOTFOUND);
    ASSERT_EQ(streamingPrivate->unsubscribeSignal(signal.getRemoteId(), "OtherSignalId"), OPENDAQ_ERR_NOTFOUND);
}

TEST_P(DirectSubscriptionTest, InvalidState)
{
    auto signal = createAndAddSignal("Signal");

    ASSERT_EQ(streamingPrivate->unsubscribeSignal(signal.getRemoteId(), nullptr), OPENDAQ_ERR_INVALIDSTATE);
}

TEST_P(DirectSubscriptionTest, SubscribeUnavailable)
{
    auto signal = createAndAddSignal("Signal");
    streaming.mock().makeSignalUnavailable("Signal");

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(streamingPrivate->subscribeSignal(signal.getRemoteId(), nullptr), OPENDAQ_SUCCESS);

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.mock().makeSignalAvailable("Signal"));
}

TEST_P(DirectSubscriptionTest, UnsubscribeUnavailable)
{
    auto signal = createAndAddSignal("Signal");

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(streamingPrivate->subscribeSignal(signal.getRemoteId(), nullptr), OPENDAQ_SUCCESS);

    streaming.mock().makeSignalUnavailable("Signal");

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(streamingPrivate->unsubscribeSignal(signal.getRemoteId(), nullptr), OPENDAQ_SUCCESS);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(0));
    ASSERT_NO_THROW(streaming.mock().makeSignalAvailable("Signal"));
}

INSTANTIATE_TEST_SUITE_P(DirectSubscription, DirectSubscriptionTest, testing::Values(true, false));

class TriggeredSubscriptionTest : public StreamingTest
{
public:
    TriggeredSubscriptionTest()
        : StreamingTest()
    {
    }

    MirroredSignalConfigPtr createAndAddSignal(const StringPtr& id)
    {
        auto signal = createWithImplementation<IMirroredSignalConfig, MockMirroredSignalImpl>(context, nullptr, id);
        signal.asPtr<IMirroredSignalPrivate>().setMirroredDataDescriptor(DataDescriptorBuilder().build());

        streaming.mock().makeSignalAvailable(id);
        EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
        streaming.ptr.addSignals({signal});

        return signal;
    }
};

TEST_F(TriggeredSubscriptionTest, BySingleConnection)
{
    auto signal = createAndAddSignal("Signal");
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());
    auto inputPort = InputPort(context, nullptr, "TestPort");

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort.connect(signal);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort.disconnect();
}

TEST_F(TriggeredSubscriptionTest, ByMultipleConnections)
{
    auto signal = createAndAddSignal("Signal");
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());
    auto inputPort1 = InputPort(context, nullptr, "TestPort1");
    auto inputPort2 = InputPort(context, nullptr, "TestPort2");

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort1.connect(signal);
    inputPort2.connect(signal);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort1.disconnect();
    inputPort2.disconnect();
}

TEST_F(TriggeredSubscriptionTest, BySingleStreamingSource)
{
    auto signal = createAndAddSignal("Signal");
    auto inputPort = InputPort(context, nullptr, "TestPort");
    inputPort.connect(signal);

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.deactivateStreaming();
}

TEST_F(TriggeredSubscriptionTest, ByRemovedStreamingSource)
{
    auto signal = createAndAddSignal("Signal");
    auto inputPort = InputPort(context, nullptr, "TestPort");
    inputPort.connect(signal);

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    streaming.ptr.removeAllSignals();
}

TEST_F(TriggeredSubscriptionTest, ByStreamedFlag)
{
    auto signal = createAndAddSignal("Signal");
    auto inputPort = InputPort(context, nullptr, "TestPort");
    inputPort.connect(signal);

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setStreamed(false);

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setStreamed(true);
}

TEST_F(TriggeredSubscriptionTest, UnsubscribeBySignalRemove)
{
    auto signal = createAndAddSignal("Signal");

    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    auto inputPort = InputPort(context, nullptr, "TestPort");

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort.connect(signal);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.remove();
}

TEST_F(TriggeredSubscriptionTest, ByMultipleStreamingSources)
{
    auto signal = createWithImplementation<IMirroredSignalConfig, MockMirroredSignalImpl>(context, nullptr, "Signal");
    signal.asPtr<IMirroredSignalPrivate>().setMirroredDataDescriptor(DataDescriptorBuilder().build());

    auto inputPort = InputPort(context, nullptr, "TestPort");
    inputPort.connect(signal);

    auto streaming1 = MockStreaming::Strict("MockStreaming1", context);
    streaming1.mock().makeSignalAvailable("Signal");
    auto streaming2 = MockStreaming::Strict("MockStreaming2", context);
    streaming2.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming1.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming1.ptr.addSignals({signal});
    EXPECT_CALL(streaming2.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming2.ptr.addSignals({signal});

    EXPECT_CALL(streaming1.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming1.ptr.getConnectionString());

    EXPECT_CALL(streaming1.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming2.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming2.ptr.getConnectionString());
}

END_NAMESPACE_OPENDAQ
