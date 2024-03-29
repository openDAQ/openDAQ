#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/gmock/streaming.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/input_port_factory.h>
#include "mock/mock_mirrored_signal.h"

BEGIN_NAMESPACE_OPENDAQ

using namespace testing;

static MirroredSignalConfigPtr createMirroredSignal(const StringPtr& id)
{
    auto signal = createWithImplementation<IMirroredSignalConfig, MockMirroredSignalImpl>(NullContext(), nullptr, id);
    signal.asPtr<IMirroredSignalPrivate>().setMirroredDataDescriptor(DataDescriptorBuilder().build());
    return signal;
}

class StreamingTest : public testing::Test
{
public:
    StreamingTest()
        : streaming(MockStreaming::Strict("MockStreaming", NullContext()))
    {}

    MockStreaming::Strict streaming;
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

class SignalTest : public StreamingTest, public WithParamInterface<bool>
{
};

TEST_P(SignalTest, AddSignalInvalid)
{
    auto signal = Signal(NullContext(), nullptr, String("Signal"));

    ASSERT_THROW(streaming.ptr.addSignals({signal}), NoInterfaceException);
}

TEST_P(SignalTest, AddSignal)
{
    auto signal = createMirroredSignal("Signal");
    if (GetParam())
        streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

TEST_P(SignalTest, AddSignalTwice)
{
    auto signal = createMirroredSignal("Signal");
    if (GetParam())
        streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
    ASSERT_THROW(streaming.ptr.addSignals({signal}), DuplicateItemException);
}

TEST_P(SignalTest, AddSignalsSameId)
{
    auto signal = createMirroredSignal("Signal");
    auto signal2 = createMirroredSignal("Signal");
    if (GetParam())
        streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(1));
    ASSERT_THROW(streaming.ptr.addSignals({signal, signal2}), DuplicateItemException);
}

TEST_P(SignalTest, AddMultipleSignals)
{
    auto signal = createMirroredSignal("Signal");
    auto signal2 = createMirroredSignal("Signal2");
    if (GetParam())
    {
        streaming.mock().makeSignalAvailable("Signal");
        streaming.mock().makeSignalAvailable("Signal2");
    }

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal, signal2}));
}

TEST_P(SignalTest, RemoveSignalInvalid)
{
    auto signal = Signal(NullContext(), nullptr, String("Signal"));

    ASSERT_THROW(streaming.ptr.removeSignals({signal}), NoInterfaceException);
}

TEST_P(SignalTest, RemoveSignalNotFound)
{
    auto signal = createMirroredSignal("Signal");
    if (GetParam())
        streaming.mock().makeSignalAvailable("Signal");

    ASSERT_THROW(streaming.ptr.removeSignals({signal}), NotFoundException);
}

TEST_P(SignalTest, RemoveSignalValid)
{
    auto signal = createMirroredSignal("Signal");
    if (GetParam())
        streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal}));
}

TEST_P(SignalTest, AddSignalAfterRemove)
{
    auto signal = createMirroredSignal("Signal");
    if (GetParam())
        streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal}));

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

TEST_P(SignalTest, RemoveMultipleSignals)
{
    auto signal = createMirroredSignal("Signal");
    auto signal2 = createMirroredSignal("Signal2");
    if (GetParam())
    {
        streaming.mock().makeSignalAvailable("Signal");
        streaming.mock().makeSignalAvailable("Signal2");
    }

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal, signal2}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal, signal2}));
}

TEST_P(SignalTest, RemoveAllNoSignals)
{
    ASSERT_NO_THROW(streaming.ptr.removeAllSignals());
}

TEST_P(SignalTest, RemoveAllOneSignal)
{
    auto signal = createMirroredSignal("Signal");
    if (GetParam())
        streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeAllSignals());

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

INSTANTIATE_TEST_SUITE_P(SignalAvailable, SignalTest, testing::Values(true, false));

class SubscriptionTest : public StreamingTest
{
};

TEST_F(SubscriptionTest, DirectSubscribeUnsubscribe)
{
    auto signal = createMirroredSignal("Signal");
    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    SubscriptionEventArgsPtr subscribeEventArgs;
    signal.getOnSubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { subscribeEventArgs = args; };

    SubscriptionEventArgsPtr unsubscribeEventArgs;
    signal.getOnUnsubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { unsubscribeEventArgs = args; };

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streaming.ptr.template asPtr<IStreamingPrivate>()->subscribeSignal(signal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );

    ASSERT_EQ(subscribeEventArgs.getStreamingConnectionString(), "MockStreaming");
    ASSERT_EQ(subscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Subscribed);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streaming.ptr.template asPtr<IStreamingPrivate>()->unsubscribeSignal(signal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );

    ASSERT_EQ(unsubscribeEventArgs.getStreamingConnectionString(), "MockStreaming");
    ASSERT_EQ(unsubscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Unsubscribed);
}

TEST_F(SubscriptionTest, DirectWithDomain)
{
    auto signal = createMirroredSignal("Signal");
    auto domainSignal = createMirroredSignal("DomainSignal");
    streaming.mock().makeSignalAvailable("Signal");
    streaming.mock().makeSignalAvailable("DomainSignal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onAddSignal(domainSignal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal, domainSignal});

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streaming.ptr.template asPtr<IStreamingPrivate>()->subscribeSignal(signal.getRemoteId(), domainSignal.getRemoteId()),
        OPENDAQ_SUCCESS
    );

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(domainSignal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streaming.ptr.template asPtr<IStreamingPrivate>()->unsubscribeSignal(signal.getRemoteId(), domainSignal.getRemoteId()),
        OPENDAQ_SUCCESS
    );
}

TEST_F(SubscriptionTest, UnsubscribeFailed)
{
    auto signal = createMirroredSignal("Signal");
    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(
        streaming.ptr.template asPtr<IStreamingPrivate>()->unsubscribeSignal(signal.getRemoteId(), nullptr),
        OPENDAQ_ERR_INVALIDSTATE
    );
}

TEST_F(SubscriptionTest, TriggeredBySingleConnection)
{
    auto signal = createMirroredSignal("Signal");

    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");

    SubscriptionEventArgsPtr subscribeEventArgs;
    signal.getOnSubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { subscribeEventArgs = args; };

    SubscriptionEventArgsPtr unsubscribeEventArgs;
    signal.getOnUnsubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { unsubscribeEventArgs = args; };

    EXPECT_CALL(streaming.mock(), onCreateDataDescriptorChangedEventPacket(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort.connect(signal);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort.disconnect();

    ASSERT_EQ(subscribeEventArgs.getStreamingConnectionString(), "MockStreaming");
    ASSERT_EQ(subscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Subscribed);

    ASSERT_EQ(unsubscribeEventArgs.getStreamingConnectionString(), "MockStreaming");
    ASSERT_EQ(unsubscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Unsubscribed);
}

TEST_F(SubscriptionTest, TriggeredByMultipleConnections)
{
    auto signal = createMirroredSignal("Signal");

    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    auto inputPort1 = InputPort(NullContext(), nullptr, "TestPort1");
    auto inputPort2 = InputPort(NullContext(), nullptr, "TestPort2");

    EXPECT_CALL(streaming.mock(), onCreateDataDescriptorChangedEventPacket(signal.getRemoteId())).Times(Exactly(2));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort1.connect(signal);
    inputPort2.connect(signal);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort1.disconnect();
    inputPort2.disconnect();
}

TEST_F(SubscriptionTest, TriggeredBySingleStreamingSource)
{
    auto signal = createMirroredSignal("Signal");
    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");
    inputPort.connect(signal);

    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.deactivateStreaming();
}

TEST_F(SubscriptionTest, TriggeredByMultipleStreamingSources)
{
    auto signal = createMirroredSignal("Signal");
    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");
    inputPort.connect(signal);

    auto streaming1 = MockStreaming::Strict("MockStreaming1", NullContext());
    streaming1.mock().makeSignalAvailable("Signal");
    auto streaming2 = MockStreaming::Strict("MockStreaming2", NullContext());
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

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming1.mock()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming2.mock()));
}

TEST_F(SubscriptionTest, TriggeredByRemovedStreamingSource)
{
    auto signal = createMirroredSignal("Signal");
    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");
    inputPort.connect(signal);

    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    streaming.ptr.removeAllSignals();
}

TEST_F(SubscriptionTest, TriggeredByStreamedFlag)
{
    auto signal = createMirroredSignal("Signal");
    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");
    inputPort.connect(signal);
    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setStreamed(false);

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.setStreamed(true);
}

TEST_F(SubscriptionTest, UnsubscribeBySignalRemove)
{
    auto signal = createMirroredSignal("Signal");

    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");

    SubscriptionEventArgsPtr unsubscribeEventArgs;
    signal.getOnUnsubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { unsubscribeEventArgs = args; };

    EXPECT_CALL(streaming.mock(), onCreateDataDescriptorChangedEventPacket(signal.getRemoteId())).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    inputPort.connect(signal);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    signal.remove();

    ASSERT_EQ(unsubscribeEventArgs.getStreamingConnectionString(), "MockStreaming");
    ASSERT_EQ(unsubscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Unsubscribed);
}

TEST_F(SubscriptionTest, SubscribeUnavailable)
{
    auto signal = createMirroredSignal("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(
        streaming.ptr.template asPtr<IStreamingPrivate>()->subscribeSignal(signal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    streaming.mock().makeSignalAvailable("Signal");
}

TEST_F(SubscriptionTest, UnsubscribeUnavailable)
{
    auto signal = createMirroredSignal("Signal");
    streaming.mock().makeSignalAvailable("Signal");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal.getRemoteId())).Times(Exactly(1));
    ASSERT_EQ(
        streaming.ptr.template asPtr<IStreamingPrivate>()->subscribeSignal(signal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );

    streaming.mock().makeSignalUnavailable("Signal");

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(0));
    ASSERT_EQ(
        streaming.ptr.template asPtr<IStreamingPrivate>()->unsubscribeSignal(signal.getRemoteId(), nullptr),
        OPENDAQ_SUCCESS
    );

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal.getRemoteId())).Times(Exactly(0));
    streaming.mock().makeSignalAvailable("Signal");
}

END_NAMESPACE_OPENDAQ
