#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mock/mock_streaming_factory.h>
#include <opendaq/signal_impl.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include "mock/mock_mirrored_signal.h"

using namespace testing;
using MirroredSignalTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

static MirroredSignalConfigPtr createMirroredSignal(const StringPtr& id)
{
    auto signal = createWithImplementation<IMirroredSignalConfig, MockMirroredSignalImpl>(NullContext(), nullptr, id);
    return signal;
}

TEST_F(MirroredSignalTest, Create)
{
    auto signal = createMirroredSignal("signal");
}

TEST_F(MirroredSignalTest, RemotelId)
{
    auto signal = createMirroredSignal("signal");
    ASSERT_EQ(signal.getRemoteId(), "signal");
}

TEST_F(MirroredSignalTest, GetActiveSourceNotAssigned)
{
    auto signal = createMirroredSignal("signal");
    ASSERT_EQ(signal.getActiveStreamingSource(), nullptr);
}

TEST_F(MirroredSignalTest, GetSourcesEmpty)
{
    auto signal = createMirroredSignal("signal");
    auto sources = signal.getStreamingSources();
    ASSERT_EQ(sources.getCount(), 0u);
}

TEST_F(MirroredSignalTest, SetActiveSourceInvalid)
{
    auto signal = createMirroredSignal("signal");
    ASSERT_THROW(signal.setActiveStreamingSource("connectionString"), NotFoundException);
}

TEST_F(MirroredSignalTest, AddSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
}

TEST_F(MirroredSignalTest, AddSourceTwice)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_ERR_DUPLICATEITEM);
}

TEST_F(MirroredSignalTest, AddGetSources)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    auto sources = signal.getStreamingSources();
    ASSERT_EQ(sources.getCount(), 1u);
    ASSERT_EQ(sources[0], "connectionString");
}

TEST_F(MirroredSignalTest, DestroyedSource)
{
    auto signal = createMirroredSignal("signal");

    {
        auto streaming = MockStreaming("connectionString");
        streaming.addSignals({signal});

        auto sources = signal.getStreamingSources();
        ASSERT_EQ(sources.getCount(), 1u);
        ASSERT_EQ(sources[0], "connectionString");
    }

    ASSERT_EQ(signal.getStreamingSources().getCount(), 0u);
}

TEST_F(MirroredSignalTest, RemoveSourceNotAdded)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->removeStreamingSource("connectionString"), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(MirroredSignalTest, AddRemoveSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_EQ(signal.getStreamingSources().getCount(), 1u);
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->removeStreamingSource("connectionString"), OPENDAQ_SUCCESS);
    ASSERT_EQ(signal.getStreamingSources().getCount(), 0u);
}

TEST_F(MirroredSignalTest, SetActiveSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_NO_THROW(signal.setActiveStreamingSource("connectionString"));

    ASSERT_EQ(signal->setActiveStreamingSource(String("connectionString")), OPENDAQ_IGNORED);
}

TEST_F(MirroredSignalTest, SetGetActiveSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    signal.setActiveStreamingSource("connectionString");
    ASSERT_EQ(signal.getActiveStreamingSource(), "connectionString");
}

TEST_F(MirroredSignalTest, RemoveActiveSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    signal.setActiveStreamingSource("connectionString");

    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->removeStreamingSource("connectionString"), OPENDAQ_SUCCESS);

    ASSERT_EQ(signal.getActiveStreamingSource(), nullptr);
}

TEST_F(MirroredSignalTest, DestroyActiveSource)
{
    auto signal = createMirroredSignal("signal");

    {
        auto streaming = MockStreaming("connectionString");
        streaming.addSignals({signal});
        signal.setActiveStreamingSource("connectionString");
    }

    ASSERT_TRUE(!signal.getActiveStreamingSource().assigned());
}

TEST_F(MirroredSignalTest, DeactivateStreaming)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming("connectionString");
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    signal.setActiveStreamingSource("connectionString");
    ASSERT_EQ(signal.getActiveStreamingSource(), "connectionString");
    ASSERT_NO_THROW(signal.deactivateStreaming());
    ASSERT_FALSE(signal.getActiveStreamingSource().assigned());
}


TEST_F(MirroredSignalTest, Streamed)
{
    auto signal = createMirroredSignal("signal");

    ASSERT_TRUE(signal.getStreamed());

    ASSERT_EQ(signal->setStreamed(true), OPENDAQ_IGNORED);
    ASSERT_EQ(signal->setStreamed(false), OPENDAQ_SUCCESS);
    ASSERT_EQ(signal->setStreamed(false), OPENDAQ_IGNORED);

    ASSERT_FALSE(signal.getStreamed());
}

END_NAMESPACE_OPENDAQ
