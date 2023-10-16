#include <opendaq/function_block_ptr.h>
#include "mock/mock_channel.h"
#include <opendaq/channel_ptr.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>


using namespace daq;

using ChannelTest = testing::Test;

TEST_F(ChannelTest, GetTags)
{
    ChannelPtr ch(MockChannel_Create(NullContext()));
    ASSERT_EQ(ch.getTags().getList().getCount(), 3u);
}
