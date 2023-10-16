#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/gmock/streaming.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_factory.h>
#include "mock/mock_signal_remote.h"

using namespace testing;
using StreamingTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

static SignalPtr createRemoteSignal(const StringPtr& id)
{
    auto signal = createWithImplementation<ISignal, MockSignalRemoteImpl>(NullContext(), nullptr, id);
    return signal;
}

TEST_F(StreamingTest, ConnectionString)
{
    auto streaming = MockStreaming::Strict();

    ASSERT_EQ(streaming.ptr.getConnectionString(), "MockStreaming");
}

TEST_F(StreamingTest, InactiveByDefault)
{
    auto streaming = MockStreaming::Strict();

    daq::Bool active;
    ASSERT_NO_THROW(active = streaming.ptr.getActive());
    ASSERT_EQ(active, False);

    EXPECT_CALL(streaming.mock(), onSetActive(_)).Times(Exactly(0));
    ASSERT_NO_THROW(streaming.ptr.setActive(False));
    ASSERT_EQ(streaming.ptr->setActive(False), OPENDAQ_IGNORED);
}

TEST_F(StreamingTest, Activate)
{
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onSetActive(true)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(True));
    ASSERT_EQ(streaming.ptr.getActive(), True);
}

TEST_F(StreamingTest, ActivateTwice)
{
    auto streaming = MockStreaming::Strict();

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
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onSetActive(true)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(True));

    EXPECT_CALL(streaming.mock(), onSetActive(false)).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.setActive(False));
    ASSERT_EQ(streaming.ptr.getActive(), False);
}

TEST_F(StreamingTest, AddSignalInvalid)
{
    auto signal = Signal(NullContext(), nullptr, String("Signal"));
    auto streaming = MockStreaming::Strict();

    ASSERT_THROW(streaming.ptr.addSignals({signal}), NoInterfaceException);
}

TEST_F(StreamingTest, AddSignalValid)
{
    auto signal = createRemoteSignal("Signal");
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onAddSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

TEST_F(StreamingTest, AddSignalTwice)
{
    auto signal = createRemoteSignal("Signal");
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onAddSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
    ASSERT_THROW(streaming.ptr.addSignals({signal}), DuplicateItemException);
}

TEST_F(StreamingTest, AddSignalsSameId)
{
    auto signal = createRemoteSignal("Signal");
    auto signal2 = createRemoteSignal("Signal");
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_THROW(streaming.ptr.addSignals({signal, signal2}), DuplicateItemException);
}

TEST_F(StreamingTest, AddMultipleSignals)
{
    auto signal = createRemoteSignal("Signal");
    auto signal2 = createRemoteSignal("Signal2");
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal, signal2}));
}

TEST_F(StreamingTest, RemoveSignalInvalid)
{
    auto signal = Signal(NullContext(), nullptr, String("Signal"));
    auto streaming = MockStreaming::Strict();

    ASSERT_THROW(streaming.ptr.removeSignals({signal}), NoInterfaceException);
}

TEST_F(StreamingTest, RemoveSignalNotFound)
{
    auto signal = createRemoteSignal("Signal");
    auto streaming = MockStreaming::Strict();

    ASSERT_THROW(streaming.ptr.removeSignals({signal}), NotFoundException);
}

TEST_F(StreamingTest, RemoveSignalValid)
{
    auto signal = createRemoteSignal("Signal");
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onAddSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal}));
}

TEST_F(StreamingTest, AddSignalAfterRemove)
{
    auto signal = createRemoteSignal("Signal");
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onAddSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal}));

    EXPECT_CALL(streaming.mock(), onAddSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

TEST_F(StreamingTest, RemoveMultipleSignals)
{
    auto signal = createRemoteSignal("Signal");
    auto signal2 = createRemoteSignal("Signal2");
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onAddSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal, signal2}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(_)).Times(Exactly(2));
    ASSERT_NO_THROW(streaming.ptr.removeSignals({signal, signal2}));
}

TEST_F(StreamingTest, RemoveAllNoSignals)
{
    auto streaming = MockStreaming::Strict();

    ASSERT_NO_THROW(streaming.ptr.removeAllSignals());
}

TEST_F(StreamingTest, RemoveAllOneSignal)
{
    auto signal = createRemoteSignal("Signal");
    auto streaming = MockStreaming::Strict();

    EXPECT_CALL(streaming.mock(), onAddSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));

    EXPECT_CALL(streaming.mock(), onRemoveSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.removeAllSignals());

    EXPECT_CALL(streaming.mock(), onAddSignal(signal.asPtr<ISignalRemote>())).Times(Exactly(1));
    ASSERT_NO_THROW(streaming.ptr.addSignals({signal}));
}

END_NAMESPACE_OPENDAQ
