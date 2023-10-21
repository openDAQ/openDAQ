#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/gmock/streaming.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/input_port_factory.h>
#include "mock/mock_mirrored_signal.h"

using namespace testing;
using StreamingTest = testing::Test;
using SubscriptionTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

static MirroredSignalConfigPtr createMirroredSignal(const StringPtr& id)
{
    auto signal = createWithImplementation<IMirroredSignalConfig, MockMirroredSignalImpl>(NullContext(), nullptr, id);
    return signal;
}

TEST_F(StreamingTest, ConnectionString)
{
    auto streaming = MockStreaming::Strict("MockStreaming");

    ASSERT_EQ(streaming.ptr.getConnectionString(), "MockStreaming");
}

TEST_F(StreamingTest, InactiveByDefault)
{
    auto streaming = MockStreaming::Strict("MockStreaming");

    daq::Bool active;
    ASSERT_NO_THROW(active = streaming.ptr.getActive());
    ASSERT_EQ(active, False);

    EXPECT_CALL(streaming.mock(), onSetActive(_)).Times(Exactly(0));
    ASSERT_NO_THROW(streaming.ptr.setActive(False));
    ASSERT_EQ(streaming.ptr->setActive(False), OPENDAQ_IGNORED);
}

TEST_F(StreamingTest, Activate)
{
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onSetActive(true)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(True));
    ASSERT_EQ(streaming.ptr.getActive(), True);
}

TEST_F(StreamingTest, ActivateTwice)
{
    auto streaming = MockStreaming::Strict("MockStreaming");

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
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onSetActive(true)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(True));

    EXPECT_CALL(streaming.mock(), onSetActive(false)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(False));
    ASSERT_EQ(streaming.ptr.getActive(), False);
}

TEST_F(StreamingTest, AddSignalInvalid)
{
    auto signal = Signal(NullContext(), nullptr, String("Signal"));
    auto streaming = MockStreaming::Strict("MockStreaming");

    ASSERT_THROW(streaming.ptr.addSignals({signal}), NoInterfaceException);
}

TEST_F(StreamingTest, AddSignalValid)
{
    auto signal = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(StreamingTest, AddSignalTwice)
{
    auto signal = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
    ASSERT_THROW(streaming.ptr.addSignals({signal}), DuplicateItemException);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(StreamingTest, AddSignalsSameId)
{
    auto signal = createMirroredSignal("Signal");
    auto signal2 = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_THROW(streaming.ptr.addSignals({signal, signal2}), DuplicateItemException);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(StreamingTest, AddMultipleSignals)
{
    auto signal = createMirroredSignal("Signal");
    auto signal2 = createMirroredSignal("Signal2");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal, signal2}));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(StreamingTest, RemoveSignalInvalid)
{
    auto signal = Signal(NullContext(), nullptr, String("Signal"));
    auto streaming = MockStreaming::Strict("MockStreaming");

    ASSERT_THROW(streaming.ptr.removeSignals({signal}), NoInterfaceException);
}

TEST_F(StreamingTest, RemoveSignalNotFound)
{
    auto signal = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    ASSERT_THROW(streaming.ptr.removeSignals({signal}), NotFoundException);
}

TEST_F(StreamingTest, RemoveSignalValid)
{
    auto signal = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal}));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(StreamingTest, AddSignalAfterRemove)
{
    auto signal = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal}));

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(StreamingTest, RemoveMultipleSignals)
{
    auto signal = createMirroredSignal("Signal");
    auto signal2 = createMirroredSignal("Signal2");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal, signal2}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal, signal2}));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(StreamingTest, RemoveAllNoSignals)
{
    auto streaming = MockStreaming::Strict("MockStreaming");

    ASSERT_NO_THROW(streaming.ptr.removeAllSignals());
}

TEST_F(StreamingTest, RemoveAllOneSignal)
{
    auto signal = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeAllSignals());

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(SubscriptionTest, DirectSubscribe)
{
    auto streaming = MockStreaming::Strict("MockStreaming");

    auto signal = createMirroredSignal("Signal");

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.template asPtr<IStreamingPrivate>()->subscribeSignal(signal));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(SubscriptionTest, DirectUnsubscribe)
{
    auto streaming = MockStreaming::Strict("MockStreaming");

    auto signal = createMirroredSignal("Signal");

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.template asPtr<IStreamingPrivate>()->unsubscribeSignal(signal));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(SubscriptionTest, TriggeredBySingleConnection)
{
    auto signal = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");

    EXPECT_CALL(streaming.mock(), onCreateDataDescriptorChangedEventPacket(signal)).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal)).Times(Exactly(1));
    inputPort.connect(signal);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal)).Times(Exactly(1));
    inputPort.disconnect();

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(SubscriptionTest, TriggeredByMultipleConnections)
{
    auto signal = createMirroredSignal("Signal");
    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    auto inputPort1 = InputPort(NullContext(), nullptr, "TestPort1");
    auto inputPort2 = InputPort(NullContext(), nullptr, "TestPort2");

    EXPECT_CALL(streaming.mock(), onCreateDataDescriptorChangedEventPacket(signal)).Times(Exactly(2));
    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal)).Times(Exactly(1));
    inputPort1.connect(signal);
    inputPort2.connect(signal);

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal)).Times(Exactly(1));
    inputPort1.disconnect();
    inputPort2.disconnect();

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(SubscriptionTest, TriggeredBySingleStreamingSource)
{
    auto signal = createMirroredSignal("Signal");
    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");
    inputPort.connect(signal);

    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal)).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal)).Times(Exactly(1));
    signal.deactivateStreaming();

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

TEST_F(SubscriptionTest, TriggeredByMultipleStreamingSources)
{
    auto signal = createMirroredSignal("Signal");
    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");
    inputPort.connect(signal);

    auto streaming1 = MockStreaming::Strict("MockStreaming1");
    auto streaming2 = MockStreaming::Strict("MockStreaming2");

    EXPECT_CALL(streaming1.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming1.ptr.addSignals({signal});
    EXPECT_CALL(streaming2.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming2.ptr.addSignals({signal});

    EXPECT_CALL(streaming1.mock(), onSubscribeSignal(signal)).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming1.ptr.getConnectionString());

    EXPECT_CALL(streaming1.mock(), onUnsubscribeSignal(signal)).Times(Exactly(1));
    EXPECT_CALL(streaming2.mock(), onSubscribeSignal(signal)).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming2.ptr.getConnectionString());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming1.mock()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming2.mock()));
}

TEST_F(SubscriptionTest, TriggeredByRemovedStreamingSource)
{
    auto signal = createMirroredSignal("Signal");
    auto inputPort = InputPort(NullContext(), nullptr, "TestPort");
    inputPort.connect(signal);

    auto streaming = MockStreaming::Strict("MockStreaming");

    EXPECT_CALL(streaming.mock(), onAddSignal(signal)).Times(Exactly(1));
    streaming.ptr.addSignals({signal});

    EXPECT_CALL(streaming.mock(), onSubscribeSignal(signal)).Times(Exactly(1));
    signal.setActiveStreamingSource(streaming.ptr.getConnectionString());

    EXPECT_CALL(streaming.mock(), onUnsubscribeSignal(signal)).Times(Exactly(1));
    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal)).Times(Exactly(1));
    streaming.ptr.removeAllSignals();

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&streaming.mock()));
}

END_NAMESPACE_OPENDAQ
