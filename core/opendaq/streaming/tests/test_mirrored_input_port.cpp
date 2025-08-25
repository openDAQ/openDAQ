#include <testutils/testutils.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/mock/mock_streaming_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/mirrored_input_port_config_ptr.h>
#include <opendaq/mirrored_input_port_private_ptr.h>
#include "mock/mock_mirrored_input_port.h"


BEGIN_NAMESPACE_OPENDAQ

class MirroredInputPortTest : public testing::Test
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


static MirroredInputPortConfigPtr createMirroredInputPort(const StringPtr& id)
{
    auto inputPort = createWithImplementation<IMirroredInputPortConfig, MockMirroredInputPortImpl>(NullContext(), nullptr, id);
    return inputPort;
}

TEST_F(MirroredInputPortTest, Create)
{
    auto inputPort = createMirroredInputPort("inputPort");
}

TEST_F(MirroredInputPortTest, RemotelId)
{
    auto inputPort = createMirroredInputPort("inputPort");
    ASSERT_EQ(inputPort.getRemoteId(), "inputPort");
}

TEST_F(MirroredInputPortTest, GetActiveSourceNotAssigned)
{
    auto inputPort = createMirroredInputPort("inputPort");
    ASSERT_EQ(inputPort.getActiveStreamingSource(), nullptr);
}

TEST_F(MirroredInputPortTest, GetSourcesEmpty)
{
    auto inputPort = createMirroredInputPort("inputPort");
    auto sources = inputPort.getStreamingSources();
    ASSERT_EQ(sources.getCount(), 0u);
}

TEST_F(MirroredInputPortTest, SetActiveSourceInvalid)
{
    auto inputPort = createMirroredInputPort("inputPort");
    ASSERT_THROW(inputPort.setActiveStreamingSource(connStr), NotFoundException);
}

TEST_F(MirroredInputPortTest, AddSource)
{
    auto inputPort = createMirroredInputPort("inputPort");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
}

TEST_F(MirroredInputPortTest, AddSourceTwice)
{
    auto inputPort = createMirroredInputPort("inputPort");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_ERROR_CODE_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->addStreamingSource(streaming), OPENDAQ_ERR_DUPLICATEITEM);
}

TEST_F(MirroredInputPortTest, AddGetSources)
{
    auto inputPort = createMirroredInputPort("inputPort");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    auto sources = inputPort.getStreamingSources();
    ASSERT_EQ(sources.getCount(), 1u);
    ASSERT_EQ(sources[0], connStr);
}

TEST_F(MirroredInputPortTest, DestroyedSource)
{
    auto inputPort = createMirroredInputPort("inputPort");

    {
        auto streaming = MockStreaming(connStr, NullContext());
        streaming.addInputPorts({inputPort});

        auto sources = inputPort.getStreamingSources();
        ASSERT_EQ(sources.getCount(), 1u);
        ASSERT_EQ(sources[0], connStr);
    }

    ASSERT_EQ(inputPort.getStreamingSources().getCount(), 0u);
}

TEST_F(MirroredInputPortTest, RemoveSourceNotAdded)
{
    auto inputPort = createMirroredInputPort("inputPort");
    ASSERT_ERROR_CODE_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->removeStreamingSource(connStr), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(MirroredInputPortTest, AddRemoveSource)
{
    auto inputPort = createMirroredInputPort("inputPort");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_EQ(inputPort.getStreamingSources().getCount(), 1u);
    ASSERT_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->removeStreamingSource(connStr), OPENDAQ_SUCCESS);
    ASSERT_EQ(inputPort.getStreamingSources().getCount(), 0u);
}

TEST_F(MirroredInputPortTest, SetGetActiveSource)
{
    auto inputPort = createMirroredInputPort("inputPort");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    ASSERT_NO_THROW(inputPort.setActiveStreamingSource(connStr));
    ASSERT_EQ(inputPort.getActiveStreamingSource(), connStr);

    ASSERT_EQ(inputPort->setActiveStreamingSource(connStr), OPENDAQ_IGNORED);
}

TEST_F(MirroredInputPortTest, RemoveActiveSource)
{
    auto inputPort = createMirroredInputPort("inputPort");
    auto streaming = MockStreaming(connStr, NullContext());
    ASSERT_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->addStreamingSource(streaming), OPENDAQ_SUCCESS);
    inputPort.setActiveStreamingSource(connStr);

    ASSERT_EQ(inputPort.template asPtr<IMirroredInputPortPrivate>()->removeStreamingSource(connStr), OPENDAQ_SUCCESS);

    ASSERT_EQ(inputPort.getActiveStreamingSource(), nullptr);
}

TEST_F(MirroredInputPortTest, DestroyActiveSource)
{
    auto inputPort = createMirroredInputPort("inputPort");

    {
        auto streaming = MockStreaming(connStr, NullContext());
        streaming.addInputPorts({inputPort});
        inputPort.setActiveStreamingSource(connStr);
    }

    ASSERT_TRUE(!inputPort.getActiveStreamingSource().assigned());
}

TEST_F(MirroredInputPortTest, Remove)
{
    auto inputPort = createMirroredInputPort("inputPort");
    auto streaming = MockStreaming(connStr, NullContext());
    streaming.addInputPorts({inputPort});
    inputPort.setActiveStreamingSource(connStr);

    ASSERT_EQ(inputPort.getStreamingSources().getCount(), 1u);
    ASSERT_EQ(inputPort.getActiveStreamingSource(), connStr);

    inputPort.remove();

    ASSERT_EQ(inputPort.getStreamingSources().getCount(), 0u);
    ASSERT_EQ(inputPort.getActiveStreamingSource(), nullptr);
    ASSERT_TRUE(inputPort.isRemoved());
}

END_NAMESPACE_OPENDAQ
