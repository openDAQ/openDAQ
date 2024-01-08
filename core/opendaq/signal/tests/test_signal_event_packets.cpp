#include <opendaq/signal_exceptions.h>
#include <opendaq/signal_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/signal_events.h>
#include <opendaq/signal_private_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/removable_ptr.h>
#include <coreobjects/property_object_class_factory.h>
#include <gtest/gtest.h>
#include <opendaq/component_private_ptr.h>
#include <opendaq/tags_factory.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_factory.h>

#include "coreobjects/property_factory.h"
#include "opendaq/event_packet_ids.h"
#include "opendaq/event_packet_ptr.h"

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

TEST_F(SignalEventPacketTest, PropertyChanged)
{
    const auto sig = Signal(NullContext(), nullptr, "sig");
    sig.addProperty(StringProperty("foo", "bar"));
    const auto ip = InputPort(NullContext(), nullptr, "ip");

    ip.connect(sig);
    ASSERT_EQ(ip.getConnection().dequeue().getType(), PacketType::Event);

    sig.setPropertyValue("foo", "test1");
    EventPacketPtr eventPacket = ip.getConnection().dequeue();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::PROPERTY_CHANGED);

    sig.setPropertyValue("foo", "test2");
    eventPacket = ip.getConnection().dequeue();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::PROPERTY_CHANGED);
}

TEST_F(SignalEventPacketTest, AttributeChanged)
{
    const auto sig = Signal(NullContext(), nullptr, "sig");
    const auto ip = InputPort(NullContext(), nullptr, "ip");

    sig.asPtr<IComponentPrivate>().unlockAllAttributes();
    ip.connect(sig);
    ASSERT_EQ(ip.getConnection().dequeue().getType(), PacketType::Event);

    sig.setVisible(false);
    EventPacketPtr eventPacket = ip.getConnection().dequeue();
    auto payload = eventPacket.getParameters();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::ATTRIBUTE_CHANGED);
    ASSERT_EQ(payload.get("Name"), "Visible");

    sig.setName("test");
    eventPacket = ip.getConnection().dequeue();
    payload = eventPacket.getParameters();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::ATTRIBUTE_CHANGED);
    ASSERT_EQ(payload.get("Name"), "Name");

    sig.setDescription("test");
    eventPacket = ip.getConnection().dequeue();
    payload = eventPacket.getParameters();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::ATTRIBUTE_CHANGED);
    ASSERT_EQ(payload.get("Name"), "Description");

    sig.setActive(false);
    eventPacket = ip.getConnection().dequeue();
    payload = eventPacket.getParameters();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::ATTRIBUTE_CHANGED);
    ASSERT_EQ(payload.get("Name"), "Active");

    sig.setPublic(false);
    eventPacket = ip.getConnection().dequeue();
    payload = eventPacket.getParameters();
    ASSERT_EQ(eventPacket.getEventId(), event_packet_id::ATTRIBUTE_CHANGED);
    ASSERT_EQ(payload.get("Name"), "Public");
}

END_NAMESPACE_OPENDAQ
