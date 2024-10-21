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
#include <opendaq/event_packet_utils.h>

using SignalEventPacketTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(SignalEventPacketTest, ValueDescriptorChanged)
{
    const auto sig = Signal(NullContext(), nullptr, "sig");
    const auto ip = InputPort(NullContext(), nullptr, "ip");

    ip.connect(sig);
    const EventPacketPtr initialEventPacket = ip.getConnection().dequeue();
    ASSERT_EQ(initialEventPacket.getType(), PacketType::Event);
    ASSERT_EQ(initialEventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_TRUE(initialEventPacket.getParameters().get("DataDescriptor").assigned());
    ASSERT_TRUE(initialEventPacket.getParameters().get("DomainDataDescriptor").assigned());
    ASSERT_EQ(initialEventPacket.getParameters().get("DataDescriptor"), NullDataDescriptor());
    ASSERT_EQ(initialEventPacket.getParameters().get("DomainDataDescriptor"), NullDataDescriptor());

    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ExplicitDataRule()).build();
    sig.setDescriptor(valueDescriptor);
    const EventPacketPtr eventPacket1 = ip.getConnection().dequeue();

    ASSERT_EQ(eventPacket1.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_TRUE(eventPacket1.getParameters().get("DataDescriptor").assigned());
    ASSERT_EQ(eventPacket1.getParameters().get("DataDescriptor"), valueDescriptor);
    ASSERT_FALSE(eventPacket1.getParameters().get("DomainDataDescriptor").assigned());

    sig.setDescriptor(nullptr);
    const EventPacketPtr eventPacket2 = ip.getConnection().dequeue();

    ASSERT_EQ(eventPacket2.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_TRUE(eventPacket2.getParameters().get("DataDescriptor").assigned());
    ASSERT_EQ(eventPacket2.getParameters().get("DataDescriptor"), NullDataDescriptor());
    ASSERT_FALSE(eventPacket2.getParameters().get("DomainDataDescriptor").assigned());
}

TEST_F(SignalEventPacketTest, DomainDescriptorChanged)
{
    const auto sig = Signal(NullContext(), nullptr, "sig");
    const auto sigDomain = Signal(NullContext(), nullptr, "sig");
    sig.setDomainSignal(sigDomain);
    const auto ip = InputPort(NullContext(), nullptr, "ip");

    ip.connect(sig);
    const EventPacketPtr initialEventPacket = ip.getConnection().dequeue();
    ASSERT_EQ(initialEventPacket.getType(), PacketType::Event);
    ASSERT_EQ(initialEventPacket.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_TRUE(initialEventPacket.getParameters().get("DataDescriptor").assigned());
    ASSERT_TRUE(initialEventPacket.getParameters().get("DomainDataDescriptor").assigned());
    ASSERT_EQ(initialEventPacket.getParameters().get("DataDescriptor"), NullDataDescriptor());
    ASSERT_EQ(initialEventPacket.getParameters().get("DomainDataDescriptor"), NullDataDescriptor());

    const auto domainDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ExplicitDataRule()).build();
    sigDomain.setDescriptor(domainDescriptor);
    const EventPacketPtr eventPacket1 = ip.getConnection().dequeue();

    ASSERT_EQ(eventPacket1.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_FALSE(eventPacket1.getParameters().get("DataDescriptor").assigned());
    ASSERT_TRUE(eventPacket1.getParameters().get("DomainDataDescriptor").assigned());
    ASSERT_EQ(eventPacket1.getParameters().get("DomainDataDescriptor"), domainDescriptor);

    sigDomain.setDescriptor(nullptr);
    const EventPacketPtr eventPacket2 = ip.getConnection().dequeue();

    ASSERT_EQ(eventPacket2.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_FALSE(eventPacket2.getParameters().get("DataDescriptor").assigned());
    ASSERT_TRUE(eventPacket2.getParameters().get("DomainDataDescriptor").assigned());
    ASSERT_EQ(eventPacket2.getParameters().get("DomainDataDescriptor"), NullDataDescriptor());
}

TEST_F(SignalEventPacketTest, EventPacketUtils)
{
    ASSERT_EQ(descriptorToEventPacketParam(nullptr), NullDataDescriptor());

    const auto descriptor = DataDescriptorBuilder().build();
    ASSERT_EQ(descriptorToEventPacketParam(descriptor), descriptor);

    ASSERT_THROW(parseDataDescriptorEventPacket(nullptr), ArgumentNullException);
    ASSERT_THROW(parseDataDescriptorEventPacket(ImplicitDomainGapDetectedEventPacket(12345)), InvalidParameterException);

    {
        const auto eventPacket = DataDescriptorChangedEventPacket(nullptr, nullptr);
        const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
            parseDataDescriptorEventPacket(eventPacket);
        ASSERT_FALSE(valueDescriptorChanged);
        ASSERT_FALSE(domainDescriptorChanged);
        ASSERT_FALSE(newValueDescriptor.assigned());
        ASSERT_FALSE(newDomainDescriptor.assigned());
    }
    {
        const auto eventPacket = DataDescriptorChangedEventPacket(NullDataDescriptor(), nullptr);
        const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
            parseDataDescriptorEventPacket(eventPacket);
        ASSERT_TRUE(valueDescriptorChanged);
        ASSERT_FALSE(domainDescriptorChanged);
        ASSERT_FALSE(newValueDescriptor.assigned());
        ASSERT_FALSE(newDomainDescriptor.assigned());
    }
    {
        const auto eventPacket = DataDescriptorChangedEventPacket(nullptr, NullDataDescriptor());
        const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
            parseDataDescriptorEventPacket(eventPacket);
        ASSERT_FALSE(valueDescriptorChanged);
        ASSERT_TRUE(domainDescriptorChanged);
        ASSERT_FALSE(newValueDescriptor.assigned());
        ASSERT_FALSE(newDomainDescriptor.assigned());
    }
    {
        const auto eventPacket = DataDescriptorChangedEventPacket(NullDataDescriptor(), NullDataDescriptor());
        const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
            parseDataDescriptorEventPacket(eventPacket);
        ASSERT_TRUE(valueDescriptorChanged);
        ASSERT_TRUE(domainDescriptorChanged);
        ASSERT_FALSE(newValueDescriptor.assigned());
        ASSERT_FALSE(newDomainDescriptor.assigned());
    }
    {
        const auto valueDescriptor = DataDescriptorBuilder().build();
        const auto domainDescriptor = DataDescriptorBuilder().build();
        const auto eventPacket = DataDescriptorChangedEventPacket(valueDescriptor, domainDescriptor);
        const auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
            parseDataDescriptorEventPacket(eventPacket);
        ASSERT_TRUE(valueDescriptorChanged);
        ASSERT_TRUE(domainDescriptorChanged);
        ASSERT_EQ(newValueDescriptor, valueDescriptor);
        ASSERT_EQ(newDomainDescriptor, domainDescriptor);
    }
}

END_NAMESPACE_OPENDAQ
