#include <opendaq/signal_factory.h>
#include "mock_channel.h"

using namespace daq;

MockChannelImpl::MockChannelImpl(ContextPtr ctx)
    : ChannelImpl<>(FunctionBlockType("mock_ch", "mock_ch", ""), std::move(ctx), nullptr, "mock")
{
    this->tags.add("tag1");
    this->tags.add("tag2");
    this->tags.add("tag3");
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockChannel, IChannel, IContext*, ctx)
