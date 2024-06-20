#include <opendaq/function_block_ptr.h>
#include "mock/mock_channel.h"
#include <opendaq/channel_ptr.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <opendaq/component_deserialize_context_factory.h>

using namespace daq;

using ChannelTest = testing::Test;

TEST_F(ChannelTest, GetTags)
{
    ChannelPtr ch(MockChannel_Create(NullContext()));
    ASSERT_EQ(ch.getTags().getList().getCount(), 3u);
}

class MockChannel final : public daq::Channel
{
public:
    MockChannel(const daq::ContextPtr& context, const daq::ComponentPtr& parent, const daq::StringPtr& localId)
        : daq::Channel(daq::FunctionBlockType("Ch", "", ""), context, parent, localId)
    {
        createAndAddSignal("sig_ch");
    }
};

TEST_F(ChannelTest, SerializeAndDeserialize)
{
    const auto ch = daq::createWithImplementation<daq::IChannel, MockChannel>(daq::NullContext(), nullptr, "Ch");
    ch.setName("ch_name");
    ch.setDescription("ch_desc");
    ch.getTags().asPtr<ITagsPrivate>().add("ch_tag");

    const auto serializer = daq::JsonSerializer(daq::True);
    ch.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = daq::JsonDeserializer();

    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "Ch");

    const daq::ChannelPtr newCh = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(newCh.getName(), ch.getName());
    ASSERT_EQ(newCh.getDescription(), ch.getDescription());
    ASSERT_EQ(newCh.getTags(), ch.getTags());

    const auto serializer2 = daq::JsonSerializer(daq::True);
    ch.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}
