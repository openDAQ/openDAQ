#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mock/mock_streaming_factory.h>
#include <opendaq/signal_impl.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/signal_remote_ptr.h>
#include "mock/mock_signal_remote.h"

using namespace testing;
using SignalRemoteTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

static SignalPtr createRemoteSignal(const StringPtr& id)
{
    auto signal = createWithImplementation<ISignal, MockSignalRemoteImpl>(NullContext(), nullptr, id);
    return signal;
}

TEST_F(SignalRemoteTest, Create)
{
    auto signal = createRemoteSignal("signal");

    SignalRemotePtr signalRemotePtr;
    ASSERT_NO_THROW(signalRemotePtr = signal.asPtr<ISignalRemote>());

    SignalConfigPtr signalConfigPtr;
    ASSERT_NO_THROW(signalConfigPtr = signal.asPtr<ISignalConfig>());
}

TEST_F(SignalRemoteTest, RemotelId)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    ASSERT_EQ(signalRemotePtr.getRemoteId(), "signal");
}

TEST_F(SignalRemoteTest, GetActiveSourceNotAssigned)
{
    auto signal = createRemoteSignal("signal");
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    ASSERT_EQ(signalConfigPtr.getActiveStreamingSource(), nullptr);
}

TEST_F(SignalRemoteTest, GetActiveSourcesEmpty)
{
    auto signal = createRemoteSignal("signal");
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    auto sources = signalConfigPtr.getStreamingSources();
    ASSERT_EQ(sources.getCount(), 0u);
}

TEST_F(SignalRemoteTest, SetActiveSourceInvalid)
{
    auto signal = createRemoteSignal("signal");
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    ASSERT_THROW(signalConfigPtr.setActiveStreamingSource("connectionString"), NotFoundException);
}

TEST_F(SignalRemoteTest, AddSource)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto streaming = MockStreaming("connectionString");
    ASSERT_NO_THROW(signalRemotePtr.addStreamingSource(streaming));
}

TEST_F(SignalRemoteTest, AddSourceTwice)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto streaming = MockStreaming("connectionString");
    signalRemotePtr.addStreamingSource(streaming);
    ASSERT_THROW(signalRemotePtr.addStreamingSource(streaming), DuplicateItemException);
}

TEST_F(SignalRemoteTest, AddGetSources)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    auto streaming = MockStreaming("connectionString");
    signalRemotePtr.addStreamingSource(streaming);

    auto sources = signalConfigPtr.getStreamingSources();
    ASSERT_EQ(sources.getCount(), 1u);
    ASSERT_EQ(sources[0], "connectionString");
}

TEST_F(SignalRemoteTest, DestroyedSource)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    {
        auto streaming = MockStreaming("connectionString");
        streaming.addSignals({signal});

        auto sources = signalConfigPtr.getStreamingSources();
        ASSERT_EQ(sources.getCount(), 1u);
        ASSERT_EQ(sources[0], "connectionString");
    }

    ASSERT_EQ(signalConfigPtr.getStreamingSources().getCount(), 0u);
}

TEST_F(SignalRemoteTest, RemoveSourceNotAdded)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto streaming = MockStreaming("connectionString");
    ASSERT_THROW(signalRemotePtr.removeStreamingSource(streaming), NotFoundException);
}

TEST_F(SignalRemoteTest, AddRemoveSource)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    auto streaming = MockStreaming("connectionString");
    signalRemotePtr.addStreamingSource(streaming);
    ASSERT_EQ(signalConfigPtr.getStreamingSources().getCount(), 1u);

    ASSERT_NO_THROW(signalRemotePtr.removeStreamingSource(streaming));
    ASSERT_EQ(signalConfigPtr.getStreamingSources().getCount(), 0u);
}

TEST_F(SignalRemoteTest, SetActiveSource)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    auto streaming = MockStreaming("connectionString");
    signalRemotePtr.addStreamingSource(streaming);
    ASSERT_NO_THROW(signalConfigPtr.setActiveStreamingSource("connectionString"));
}

TEST_F(SignalRemoteTest, SetGetActiveSource)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    auto streaming = MockStreaming("connectionString");
    signalRemotePtr.addStreamingSource(streaming);
    signalConfigPtr.setActiveStreamingSource("connectionString");
    ASSERT_EQ(signalConfigPtr.getActiveStreamingSource(), "connectionString");
}

TEST_F(SignalRemoteTest, RemoveActiveSource)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    auto streaming = MockStreaming("connectionString");
    signalRemotePtr.addStreamingSource(streaming);
    signalConfigPtr.setActiveStreamingSource("connectionString");

    ASSERT_NO_THROW(signalRemotePtr.removeStreamingSource(streaming));

    ASSERT_EQ(signalConfigPtr.getActiveStreamingSource(), nullptr);
}

TEST_F(SignalRemoteTest, DestroyedActiveSource)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    {
        auto streaming = MockStreaming("connectionString");
        streaming.addSignals({signal});
        signalConfigPtr.setActiveStreamingSource("connectionString");
    }

    ASSERT_EQ(signalConfigPtr.getActiveStreamingSource(), nullptr);
}

TEST_F(SignalRemoteTest, DeactivateStreaming)
{
    auto signal = createRemoteSignal("signal");
    auto signalRemotePtr = signal.asPtr<ISignalRemote>();
    auto signalConfigPtr = signal.asPtr<ISignalConfig>();
    auto streaming = MockStreaming("connectionString");
    signalRemotePtr.addStreamingSource(streaming);
    signalConfigPtr.setActiveStreamingSource("connectionString");
    ASSERT_EQ(signalConfigPtr.getActiveStreamingSource(), "connectionString");
    ASSERT_NO_THROW(signalConfigPtr.deactivateStreaming());
    ASSERT_FALSE(signalConfigPtr.getActiveStreamingSource().assigned());
}

END_NAMESPACE_OPENDAQ
