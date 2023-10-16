#include <gtest/gtest.h>

#include <opendaq/event_packet_ptr.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_descriptor_factory.h>

using EventPacketTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(EventPacketTest, Create)
{
    const auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    ASSERT_NO_THROW(EventPacket("id", Dict<IString,IBaseObject>()));
    ASSERT_NO_THROW(DataDescriptorChangedEventPacket(desc, desc));
}

TEST_F(EventPacketTest, Getters)
{
    const auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    auto packet1 = DataDescriptorChangedEventPacket(desc, desc);
    auto packet2 = EventPacket("id", Dict<IString,IBaseObject>());

    ASSERT_EQ(packet1.getEventId(), event_packet_id::DATA_DESCRIPTOR_CHANGED);
    ASSERT_EQ(packet1.getParameters().get(event_packet_param::DATA_DESCRIPTOR), desc);
    ASSERT_EQ(packet1.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR), desc);
    ASSERT_EQ(packet1.getType(), PacketType::Event);
    ASSERT_EQ(packet2.getType(), PacketType::Event);
}

TEST_F(EventPacketTest, Equals)
{
    const auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto packet = DataDescriptorChangedEventPacket(desc, desc);

    const auto desc1 = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto packet1 = DataDescriptorChangedEventPacket(desc1, desc1);

    ASSERT_EQ(packet1, packet);
}

TEST_F(EventPacketTest, SerializeDeserialize)
{
    const auto desc = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();
    const auto packet = DataDescriptorChangedEventPacket(desc, desc);

    const auto serializer = JsonSerializer(False);
    packet.serialize(serializer);

    const auto serialized = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const EventPacketPtr packet1 = deserializer.deserialize(serialized.toStdString());

    ASSERT_EQ(packet1, packet);
}

END_NAMESPACE_OPENDAQ
