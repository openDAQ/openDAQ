#include <opendaq/signal_exceptions.h>
#include <opendaq/signal_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_private_ptr.h>
#include <opendaq/context_factory.h>
#include <gtest/gtest.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/input_port_factory.h>
#include <coreobjects/property_factory.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_ptr.h>

using SignalEventPacketTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(SignalEventPacketTest, DescriptorChanged1)
{
    const auto sig = Signal(NullContext(), nullptr, "sig");
    const auto ip = InputPort(NullContext(), nullptr, "ip");

    ip.connect(sig);
    ASSERT_EQ(ip.getConnection().dequeue().getType(), PacketType::Event);

    sig.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ExplicitDataRule()).build());
    const EventPacketPtr eventPacket = ip.getConnection().dequeue();

    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_EQ(eventPacket.getParameters().get("DataDescriptor").assigned(), true);
    ASSERT_EQ(eventPacket.getParameters().get("DomainDataDescriptor").assigned(), false);
}

TEST_F(SignalEventPacketTest, DescriptorChanged2)
{
    const auto sig = Signal(NullContext(), nullptr, "sig");
    const auto sigDomain = Signal(NullContext(), nullptr, "sig");
    sig.setDomainSignal(sigDomain);
    const auto ip = InputPort(NullContext(), nullptr, "ip");

    ip.connect(sig);
    ASSERT_EQ(ip.getConnection().dequeue().getType(), PacketType::Event);

    sigDomain.setDescriptor(DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ExplicitDataRule()).build());
    const EventPacketPtr eventPacket = ip.getConnection().dequeue();

    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_EQ(eventPacket.getParameters().get("DataDescriptor").assigned(), false);
    ASSERT_EQ(eventPacket.getParameters().get("DomainDataDescriptor").assigned(), true);
}

END_NAMESPACE_OPENDAQ
