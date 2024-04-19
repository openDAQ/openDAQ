#include <gtest/gtest.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mock/mock_streaming_factory.h>
#include <opendaq/signal_impl.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/mirrored_signal_private_ptr.h>
#include "mock/mock_mirrored_signal.h"


BEGIN_NAMESPACE_OPENDAQ

class MirroredSignalTest : public testing::Test
{
protected:

    void SetUp() override
    {
        connStr = "connectionString";
    }

    void TearDown() override
    {
    }

    StringPtr connStr;
};


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
    ASSERT_THROW(signal.setActiveStreamingSource(connStr), NotFoundException);
}

TEST_F(MirroredSignalTest, AddSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
}

TEST_F(MirroredSignalTest, AddSourceTwice)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_ERR_DUPLICATEITEM);
}

TEST_F(MirroredSignalTest, AddGetSources)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    auto sources = signal.getStreamingSources();
    ASSERT_EQ(sources.getCount(), 1u);
    ASSERT_EQ(sources[0], connStr);
}

TEST_F(MirroredSignalTest, DestroyedSource)
{
    auto signal = createMirroredSignal("signal");

    {
        auto streaming = MockStreaming(connStr, NullContext());
        streaming.addSignals({signal});

        auto sources = signal.getStreamingSources();
        ASSERT_EQ(sources.getCount(), 1u);
        ASSERT_EQ(sources[0], connStr);
    }

    ASSERT_EQ(signal.getStreamingSources().getCount(), 0u);
}

TEST_F(MirroredSignalTest, RemoveSourceNotAdded)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->removeStreamingSource(connStr), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(MirroredSignalTest, AddRemoveSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_EQ(signal.getStreamingSources().getCount(), 1u);
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->removeStreamingSource(connStr), OPENDAQ_SUCCESS);
    ASSERT_EQ(signal.getStreamingSources().getCount(), 0u);
}

TEST_F(MirroredSignalTest, SetActiveSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_NO_THROW(signal.setActiveStreamingSource(connStr));

    ASSERT_EQ(signal->setActiveStreamingSource(connStr), OPENDAQ_IGNORED);
}

TEST_F(MirroredSignalTest, SetGetActiveSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    signal.setActiveStreamingSource(connStr);
    ASSERT_EQ(signal.getActiveStreamingSource(), connStr);
}

TEST_F(MirroredSignalTest, RemoveActiveSource)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    signal.setActiveStreamingSource(connStr);

    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->removeStreamingSource(connStr), OPENDAQ_SUCCESS);

    ASSERT_EQ(signal.getActiveStreamingSource(), nullptr);
}

TEST_F(MirroredSignalTest, DestroyActiveSource)
{
    auto signal = createMirroredSignal("signal");

    {
        auto streaming = MockStreaming(connStr, NullContext());
        streaming.addSignals({signal});
        signal.setActiveStreamingSource(connStr);
    }

    ASSERT_TRUE(!signal.getActiveStreamingSource().assigned());
}

TEST_F(MirroredSignalTest, DeactivateStreaming)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(signal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    signal.setActiveStreamingSource(connStr);
    ASSERT_EQ(signal.getActiveStreamingSource(), connStr);
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

TEST_F(MirroredSignalTest, SubscriptionEvents)
{
    auto signal = createMirroredSignal("signal");

    SubscriptionEventArgsPtr subscribeEventArgs;
    signal.getOnSubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { subscribeEventArgs = args; };

    SubscriptionEventArgsPtr unsubscribeEventArgs;
    signal.getOnUnsubscribeComplete() +=
        [&](MirroredSignalConfigPtr& sender, SubscriptionEventArgsPtr& args) { unsubscribeEventArgs = args; };

    signal.template asPtr<IMirroredSignalPrivate>()->subscribeCompleted(connStr);
    ASSERT_EQ(subscribeEventArgs.getStreamingConnectionString(), connStr);
    ASSERT_EQ(subscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Subscribed);

    signal.template asPtr<IMirroredSignalPrivate>()->unsubscribeCompleted(connStr);
    ASSERT_EQ(unsubscribeEventArgs.getStreamingConnectionString(), connStr);
    ASSERT_EQ(unsubscribeEventArgs.getSubscriptionEventType(), SubscriptionEventType::Unsubscribed);
}

TEST_F(MirroredSignalTest, Remove)
{
    auto signal = createMirroredSignal("signal");
    auto streaming = MockStreaming(connStr, NullContext());
    streaming.addSignals({signal});
    signal.setActiveStreamingSource(connStr);

    ASSERT_EQ(signal.getStreamingSources().getCount(), 1u);
    ASSERT_EQ(signal.getActiveStreamingSource(), connStr);

    signal.remove();

    ASSERT_EQ(signal.getStreamingSources().getCount(), 0u);
    ASSERT_EQ(signal.getActiveStreamingSource(), nullptr);
    ASSERT_TRUE(signal.isRemoved());
}

END_NAMESPACE_OPENDAQ
