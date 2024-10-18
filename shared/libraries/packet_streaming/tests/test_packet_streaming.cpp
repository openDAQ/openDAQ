#include <gtest/gtest.h>
#include <packet_streaming/packet_streaming_client.h>
#include <packet_streaming/packet_streaming_server.h>
#include <opendaq/packet_factory.h>
#include <opendaq/data_descriptor_factory.h>
#include <opendaq/data_rule_factory.h>
#include <opendaq/packet_destruct_callback_factory.h>
#include "packet_transmission.h"

using namespace daq;
using namespace packet_streaming;

class PacketStreamingTest : public testing::Test
{
public:
    void SetUp() override
    {
    }

protected:
    PacketStreamingServer server {10};
    PacketStreamingClient client;
    PacketTransmission transmission;

    void transmitAll()
    {
        while (const auto serverPacketBuffer = server.getNextPacketBuffer())
        {
            transmission.sendPacketBuffer(serverPacketBuffer);
            while (const auto clientPacketBuffer = transmission.recvPacketBuffer())
                client.addPacketBuffer(clientPacketBuffer);
        }
    }

    void completeTransmitAll()
    {
        server.checkAndSendReleasePacket(true);
        transmitAll();
    }
};

TEST_F(PacketStreamingTest, DataDescChangedEventPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    const auto domainDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Int64).build();
    const auto serverEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, domainDescriptor);

    server.addDaqPacket(1, serverEventPacket);
    const auto serverPacketBuffer = server.getNextPacketBuffer();

    transmission.sendPacketBuffer(serverPacketBuffer);
    const auto clientPacketBuffer = transmission.recvPacketBuffer();

    client.addPacketBuffer(clientPacketBuffer);
    auto [signalId, clientEventPacket] = client.getNextDaqPacket();

    ASSERT_EQ(signalId, 1u);
    ASSERT_EQ(serverEventPacket, clientEventPacket);

    ASSERT_TRUE(client.areReferencesCleared());
}

TEST_F(PacketStreamingTest, DataDescNullEventPacket)
{
    const auto serverEventPacket = DataDescriptorChangedEventPacket(NullDataDescriptor(), NullDataDescriptor());

    server.addDaqPacket(1, serverEventPacket);
    const auto serverPacketBuffer = server.getNextPacketBuffer();

    transmission.sendPacketBuffer(serverPacketBuffer);
    const auto clientPacketBuffer = transmission.recvPacketBuffer();

    client.addPacketBuffer(clientPacketBuffer);
    auto [signalId, clientEventPacket] = client.getNextDaqPacket();

    ASSERT_EQ(signalId, 1u);
    ASSERT_EQ(serverEventPacket, clientEventPacket);

    ASSERT_TRUE(client.areReferencesCleared());
}

TEST_F(PacketStreamingTest, DataPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto serverDataDescriptorChangedEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    server.addDaqPacket(1, serverDataDescriptorChangedEventPacket);

    constexpr size_t sampleCount = 100;
    auto serverDataPacket = DataPacket(valueDescriptor, sampleCount, 1024);
    auto data = static_cast<float*>(serverDataPacket.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data++ = static_cast<float>(i);

    server.addDaqPacket(1, serverDataPacket);

    transmitAll();

    auto [signalIdDataDescriptorChangedEventPacket, clientDataDescriptorChangedEventPacket] = client.getNextDaqPacket();
    auto [signalIdOfDataPacket, clientDataPacket] = client.getNextDaqPacket();

    ASSERT_EQ(signalIdDataDescriptorChangedEventPacket, 1u);
    ASSERT_EQ(signalIdOfDataPacket, 1u);
    ASSERT_EQ(serverDataDescriptorChangedEventPacket, clientDataDescriptorChangedEventPacket);
    ASSERT_EQ(serverDataPacket, clientDataPacket);

    serverDataPacket.release();

    completeTransmitAll();
    ASSERT_TRUE(client.areReferencesCleared());
}

TEST_F(PacketStreamingTest, ConstantRuleDataPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(ConstantDataRule()).build();

    const auto serverDataDescriptorChangedEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    server.addDaqPacket(1, serverDataDescriptorChangedEventPacket);

    constexpr size_t sampleCount = 100;
    auto serverDataPacket = ConstantDataPacketWithDomain(nullptr, valueDescriptor, sampleCount, 24.0f);

    server.addDaqPacket(1, serverDataPacket);

    transmitAll();

    auto [signalIdDataDescriptorChangedEventPacket, clientDataDescriptorChangedEventPacket] = client.getNextDaqPacket();
    auto [signalIdOfDataPacket, clientDataPacket] = client.getNextDaqPacket();

    ASSERT_EQ(signalIdDataDescriptorChangedEventPacket, 1u);
    ASSERT_EQ(signalIdOfDataPacket, 1u);
    ASSERT_EQ(serverDataDescriptorChangedEventPacket, clientDataDescriptorChangedEventPacket);

    const auto clientData = reinterpret_cast<float*>(clientDataPacket.asPtr<IDataPacket>(true).getData());

    ASSERT_EQ(clientDataPacket.asPtr<IDataPacket>(true).getSampleCount(), sampleCount);
    for (size_t i = 0; i < sampleCount; i++)
        ASSERT_EQ(clientData[i], 24);

    serverDataPacket.release();

    completeTransmitAll();
    ASSERT_TRUE(client.areReferencesCleared());
}


TEST_F(PacketStreamingTest, CanReleaseDataPacket)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto serverDataDescriptorChangedEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, nullptr);
    server.addDaqPacket(1, serverDataDescriptorChangedEventPacket);

    constexpr size_t sampleCount = 100;
    auto serverDataPacket = DataPacket(valueDescriptor, sampleCount, 1024);
    auto data = static_cast<float*>(serverDataPacket.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data++ = static_cast<float>(i);

    bool serverDataPacketDestroyed = false;
    serverDataPacket.subscribeForDestructNotification(
        PacketDestructCallback([&serverDataPacketDestroyed] { serverDataPacketDestroyed = true; }));

    server.addDaqPacket(1, std::move(serverDataPacket));

    transmitAll();

    ASSERT_TRUE(serverDataPacketDestroyed);

    auto [signalIdDataDescriptorChangedEventPacket, clientDataDescriptorChangedEventPacket] = client.getNextDaqPacket();
    auto [signalIdOfDataPacket, clientDataPacket] = client.getNextDaqPacket();

    ASSERT_EQ(signalIdDataDescriptorChangedEventPacket, 1u);
    ASSERT_EQ(signalIdOfDataPacket, 1u);
}


TEST_F(PacketStreamingTest, DataPacketsWithDataDescriptorChanged)
{
    const auto valueDescriptor1 = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();
    const auto serverDataDescriptorChangedEventPacket1 = DataDescriptorChangedEventPacket(valueDescriptor1, nullptr);
    server.addDaqPacket(1, serverDataDescriptorChangedEventPacket1);

    constexpr size_t sampleCount1 = 100;
    auto serverDataPacket1 = DataPacket(valueDescriptor1, sampleCount1, 1024);
    auto data1 = static_cast<float*>(serverDataPacket1.getRawData());
    for (size_t i = 0; i < sampleCount1; i++)
        *data1++ = static_cast<float>(i);
    server.addDaqPacket(1, serverDataPacket1);

    constexpr size_t sampleCount2 = 101;
    auto serverDataPacket2 = DataPacket(valueDescriptor1, sampleCount2, 2048);
    auto data2 = static_cast<float*>(serverDataPacket2.getRawData());
    for (size_t i = 0; i < sampleCount2; i++)
        *data2++ = static_cast<float>(i);
    server.addDaqPacket(1, serverDataPacket2);

    const auto valueDescriptor2 = DataDescriptorBuilder().setSampleType(SampleType::Int16).build();
    const auto serverDataDescriptorChangedEventPacket2 = DataDescriptorChangedEventPacket(valueDescriptor2, nullptr);
    server.addDaqPacket(1, serverDataDescriptorChangedEventPacket2);

    constexpr size_t sampleCount3 = 10;
    auto serverDataPacket3 = DataPacket(valueDescriptor2, sampleCount3, 3072);
    auto data3 = static_cast<int16_t*>(serverDataPacket3.getRawData());
    for (size_t i = 0; i < sampleCount3; i++)
        *data3++ = static_cast<int16_t>(i);
    server.addDaqPacket(1, serverDataPacket3);

    constexpr size_t sampleCount4 = 11;
    auto serverDataPacket4 = DataPacket(valueDescriptor2, sampleCount4, 4096);
    auto data4 = static_cast<int16_t*>(serverDataPacket4.getRawData());
    for (size_t i = 0; i < sampleCount4; i++)
        *data4++ = static_cast<int16_t>(i);
    server.addDaqPacket(1, serverDataPacket4);

    transmitAll();

    auto [signalId1, packet1] = client.getNextDaqPacket();
    ASSERT_EQ(signalId1, 1u);
    ASSERT_EQ(serverDataDescriptorChangedEventPacket1, packet1);

    auto [signalId2, packet2] = client.getNextDaqPacket();
    ASSERT_EQ(signalId2, 1u);
    ASSERT_EQ(serverDataPacket1, packet2);

    auto [signalId3, packet3] = client.getNextDaqPacket();
    ASSERT_EQ(signalId3, 1u);
    ASSERT_EQ(serverDataPacket2, packet3);

    auto [signalId4, packet4] = client.getNextDaqPacket();
    ASSERT_EQ(signalId4, 1u);
    ASSERT_EQ(serverDataDescriptorChangedEventPacket2, packet4);

    auto [signalId5, packet5] = client.getNextDaqPacket();
    ASSERT_EQ(signalId5, 1u);
    ASSERT_EQ(serverDataPacket3, packet5);

    auto [signalId6, packet6] = client.getNextDaqPacket();
    ASSERT_EQ(signalId6, 1u);
    ASSERT_EQ(serverDataPacket4, packet6);

    auto [signalId7, packet7] = client.getNextDaqPacket();
    ASSERT_EQ(signalId7, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet7);

    serverDataPacket1.release();
    serverDataPacket2.release();
    serverDataPacket3.release();
    serverDataPacket4.release();

    completeTransmitAll();
    ASSERT_TRUE(client.areReferencesCleared());
}

TEST_F(PacketStreamingTest, DataPacketTwice)
{
    constexpr uint32_t valueSignal1 = 1;
    constexpr uint32_t valueSignal2 = 2;
    constexpr uint32_t domainSignal = 3;

    const auto valueDescriptor1 = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto valueDescriptor2 = DataDescriptorBuilder().setSampleType(SampleType::Int16).build();

    const auto domainDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(LinearDataRule(1, 0)).setTickResolution(Ratio(1, 1000)).build();

    const auto serverDataDescriptorChangedOnValueSignal1EventPacket = DataDescriptorChangedEventPacket(valueDescriptor1, domainDescriptor);
    server.addDaqPacket(valueSignal1, serverDataDescriptorChangedOnValueSignal1EventPacket);

    const auto serverDataDescriptorChangedOnValueSignal2EventPacket = DataDescriptorChangedEventPacket(valueDescriptor2, domainDescriptor);
    server.addDaqPacket(valueSignal2, serverDataDescriptorChangedOnValueSignal2EventPacket);

    const auto serverDataDescriptorChangedOnDomainSignalEventPacket = DataDescriptorChangedEventPacket(domainDescriptor, nullptr);
    server.addDaqPacket(domainSignal, serverDataDescriptorChangedOnDomainSignalEventPacket);

    constexpr size_t sampleCount = 100;

    auto serverDomainDataPacket = DataPacket(domainDescriptor, sampleCount, 1024);

    auto serverValueDataPacket = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor1, sampleCount);
    auto data1 = static_cast<float*>(serverValueDataPacket.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data1++ = static_cast<float>(i);

    server.addDaqPacket(valueSignal1, serverValueDataPacket);
    transmitAll();
    server.addDaqPacket(valueSignal2, serverValueDataPacket);
    transmitAll();
    server.addDaqPacket(domainSignal, serverDomainDataPacket);
    transmitAll();

    auto [signalId1, packet1] = client.getNextDaqPacket();
    ASSERT_EQ(signalId1, valueSignal1);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignal1EventPacket, packet1);

    auto [signalId2, packet2] = client.getNextDaqPacket();
    ASSERT_EQ(signalId2, valueSignal2);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignal2EventPacket, packet2);

    auto [signalId3, packet3] = client.getNextDaqPacket();
    ASSERT_EQ(signalId3, domainSignal);
    ASSERT_EQ(serverDataDescriptorChangedOnDomainSignalEventPacket, packet3);

    auto [signalId4, packet4] = client.getNextDaqPacket();
    ASSERT_EQ(signalId4, domainSignal);
    ASSERT_EQ(serverDomainDataPacket, packet4);

    auto [signalId5, packet5] = client.getNextDaqPacket();
    ASSERT_EQ(signalId5, valueSignal1);
    ASSERT_EQ(serverValueDataPacket, packet5);

    auto [signalId6, packet6] = client.getNextDaqPacket();
    ASSERT_EQ(signalId6, valueSignal2);
    ASSERT_EQ(serverValueDataPacket, packet6);

    server.checkAndSendReleasePacket(true);

    transmitAll();

    auto [signalId10, packet10] = client.getNextDaqPacket();
    ASSERT_EQ(signalId10, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet10);
}

class DataWithDomainPacketStreamingTest : public PacketStreamingTest, public testing::WithParamInterface<bool>
{
};

TEST_P(DataWithDomainPacketStreamingTest, DataPacketWithDomain)
{
    const bool valuePacketFirst = GetParam();

    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto domainDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(LinearDataRule(1, 0)).setTickResolution(Ratio(1, 1000)).build();

    const auto serverDataDescriptorChangedOnValueSignalEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, domainDescriptor);
    server.addDaqPacket(1, serverDataDescriptorChangedOnValueSignalEventPacket);

    const auto serverDataDescriptorChangedOnDomainSignalEventPacket = DataDescriptorChangedEventPacket(domainDescriptor, nullptr);
    server.addDaqPacket(2, serverDataDescriptorChangedOnDomainSignalEventPacket);

    constexpr size_t sampleCount = 100;

    auto serverDomainDataPacket = DataPacket(domainDescriptor, sampleCount, 1024);
    auto serverValueDataPacket = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor, sampleCount);
    auto data = static_cast<float*>(serverValueDataPacket.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data++ = static_cast<float>(i);

    if (valuePacketFirst)
    {
        server.addDaqPacket(1, serverValueDataPacket);
        server.addDaqPacket(2, serverDomainDataPacket);
    }
    else
    {
        server.addDaqPacket(2, serverDomainDataPacket);
        server.addDaqPacket(1, serverValueDataPacket);
    }

    transmitAll();

    auto [signalId1, packet1] = client.getNextDaqPacket();
    ASSERT_EQ(signalId1, 1u);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignalEventPacket, packet1);

    auto [signalId2, packet2] = client.getNextDaqPacket();
    ASSERT_EQ(signalId2, 2u);
    ASSERT_EQ(serverDataDescriptorChangedOnDomainSignalEventPacket, packet2);

    auto [signalId3, packet3] = client.getNextDaqPacket();
    bool packet3destroyed = false;
    packet3.subscribeForDestructNotification(PacketDestructCallback([&packet3destroyed] { packet3destroyed = true; }));

    ASSERT_EQ(signalId3, 2u);
    ASSERT_EQ(serverDomainDataPacket, packet3);

    auto [signalId4, packet4] = client.getNextDaqPacket();
    bool packet4destroyed = false;
    packet3.subscribeForDestructNotification(PacketDestructCallback([&packet4destroyed] { packet4destroyed = true; }));

    ASSERT_EQ(signalId4, 1u);
    ASSERT_EQ(serverValueDataPacket, packet4);

    auto [signalId5, packet5] = client.getNextDaqPacket();
    ASSERT_EQ(signalId5, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet5);

    serverDomainDataPacket.release();
    serverValueDataPacket.release();

    packet3.release();
    ASSERT_FALSE(packet3destroyed);

    packet4.release();
    ASSERT_FALSE(packet4destroyed);

    server.checkAndSendReleasePacket(true);

    transmitAll();

    auto [signalId6, packet6] = client.getNextDaqPacket();
    ASSERT_EQ(signalId6, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet6);

    ASSERT_TRUE(packet3destroyed);
    ASSERT_TRUE(packet4destroyed);

    ASSERT_TRUE(client.areReferencesCleared());
}

INSTANTIATE_TEST_SUITE_P(ValuePacketFirst, DataWithDomainPacketStreamingTest, testing::Values(true, false));


class SharedDomainPacketStreamingTest : public PacketStreamingTest, public testing::WithParamInterface<int>
{
};

TEST_P(SharedDomainPacketStreamingTest, SharedDomainPacket)
{
    const auto domainPacketPosition = GetParam();

    const auto valueDescriptor1 = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto valueDescriptor2 = DataDescriptorBuilder().setSampleType(SampleType::Int16).build();

    const auto domainDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Float32).setRule(LinearDataRule(1, 0)).setTickResolution(Ratio(1, 1000)).build();

    const auto serverDataDescriptorChangedOnValueSignal1EventPacket = DataDescriptorChangedEventPacket(valueDescriptor1, domainDescriptor);
    server.addDaqPacket(1, serverDataDescriptorChangedOnValueSignal1EventPacket);

    const auto serverDataDescriptorChangedOnValueSignal2EventPacket = DataDescriptorChangedEventPacket(valueDescriptor2, domainDescriptor);
    server.addDaqPacket(2, serverDataDescriptorChangedOnValueSignal2EventPacket);

    const auto serverDataDescriptorChangedOnDomainSignalEventPacket = DataDescriptorChangedEventPacket(domainDescriptor, nullptr);
    server.addDaqPacket(3, serverDataDescriptorChangedOnDomainSignalEventPacket);

    constexpr size_t sampleCount = 100;

    auto serverDomainDataPacket = DataPacket(domainDescriptor, sampleCount, 1024);

    auto serverValueDataPacket1 = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor1, sampleCount);
    auto data1 = static_cast<float*>(serverValueDataPacket1.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data1++ = static_cast<float>(i);

    auto serverValueDataPacket2 = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor2, sampleCount);
    auto data2 = static_cast<int16_t*>(serverValueDataPacket2.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data2++ = static_cast<int16_t>(i);

    switch (domainPacketPosition)
    {
        case 0:
            server.addDaqPacket(3, serverDomainDataPacket);
            server.addDaqPacket(1, serverValueDataPacket1);
            server.addDaqPacket(2, serverValueDataPacket2);
            break;
        case 1:
            server.addDaqPacket(1, serverValueDataPacket1);
            server.addDaqPacket(3, serverDomainDataPacket);
            server.addDaqPacket(2, serverValueDataPacket2);
            break;
        case 2:
            server.addDaqPacket(1, serverValueDataPacket1);
            server.addDaqPacket(2, serverValueDataPacket2);
            server.addDaqPacket(3, serverDomainDataPacket);
            break;
        default:
            ASSERT_TRUE(false);
    }

    transmitAll();

    auto [signalId1, packet1] = client.getNextDaqPacket();
    ASSERT_EQ(signalId1, 1u);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignal1EventPacket, packet1);

    auto [signalId2, packet2] = client.getNextDaqPacket();
    ASSERT_EQ(signalId2, 2u);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignal2EventPacket, packet2);

    auto [signalId3, packet3] = client.getNextDaqPacket();
    ASSERT_EQ(signalId3, 3u);
    ASSERT_EQ(serverDataDescriptorChangedOnDomainSignalEventPacket, packet3);

    auto [signalId4, packet4] = client.getNextDaqPacket();
    bool packet4destroyed = false;
    packet4.subscribeForDestructNotification(PacketDestructCallback([&packet4destroyed] { packet4destroyed = true; }));

    ASSERT_EQ(signalId4, 3u);
    ASSERT_EQ(serverDomainDataPacket, packet4);

    auto [signalId5, packet5] = client.getNextDaqPacket();
    bool packet5destroyed = false;
    packet5.subscribeForDestructNotification(PacketDestructCallback([&packet5destroyed] { packet5destroyed = true; }));

    ASSERT_EQ(signalId5, 1u);
    ASSERT_EQ(serverValueDataPacket1, packet5);

    auto [signalId6, packet6] = client.getNextDaqPacket();
    bool packet6destroyed = false;
    packet6.subscribeForDestructNotification(PacketDestructCallback([&packet6destroyed] { packet6destroyed = true; }));

    ASSERT_EQ(signalId6, 2u);
    ASSERT_EQ(serverValueDataPacket2, packet6);

    auto [signalId7, packet7] = client.getNextDaqPacket();
    ASSERT_EQ(signalId7, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet7);

    serverDomainDataPacket.release();
    serverValueDataPacket1.release();
    serverValueDataPacket2.release();

    packet4.release();
    ASSERT_FALSE(packet4destroyed);

    packet5.release();
    ASSERT_FALSE(packet5destroyed);

    packet6.release();
    ASSERT_FALSE(packet6destroyed);

    server.checkAndSendReleasePacket(true);

    transmitAll();

    auto [signalId8, packet8] = client.getNextDaqPacket();
    ASSERT_EQ(signalId8, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet8);

    ASSERT_TRUE(packet4destroyed);
    ASSERT_TRUE(packet5destroyed);
    ASSERT_TRUE(packet6destroyed);

    ASSERT_TRUE(client.areReferencesCleared());
}

INSTANTIATE_TEST_SUITE_P(DomainOrder, SharedDomainPacketStreamingTest, testing::Values(0, 1, 2));

class ValuePacketDestroyedBeforeDomainSentTest : public PacketStreamingTest, public testing::WithParamInterface<bool>
{
};

TEST_P(ValuePacketDestroyedBeforeDomainSentTest, ValuePacketDestroyedBeforeDomainSent)
{
    const auto valueDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto domainDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(LinearDataRule(1, 0)).setTickResolution(Ratio(1, 1000)).build();

    const auto serverDataDescriptorChangedOnValueSignalEventPacket = DataDescriptorChangedEventPacket(valueDescriptor, domainDescriptor);
    server.addDaqPacket(1, serverDataDescriptorChangedOnValueSignalEventPacket);

    const auto serverDataDescriptorChangedOnDomainSignalEventPacket = DataDescriptorChangedEventPacket(domainDescriptor, nullptr);
    server.addDaqPacket(2, serverDataDescriptorChangedOnDomainSignalEventPacket);

    constexpr size_t sampleCount = 100;

    auto serverDomainDataPacket = DataPacket(domainDescriptor, sampleCount, 1024);

    auto serverValueDataPacket = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor, sampleCount);
    auto data1 = static_cast<float*>(serverValueDataPacket.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data1++ = static_cast<float>(i);

    const auto movePacket = GetParam();

    if (movePacket)
        server.addDaqPacket(1, std::move(serverValueDataPacket));
    else
    {
        server.addDaqPacket(1, serverValueDataPacket);
        serverValueDataPacket.release();
    }
    transmitAll();
    server.checkAndSendReleasePacket(true);
    server.addDaqPacket(2, serverDomainDataPacket);
    transmitAll();

    auto [signalId1, packet1] = client.getNextDaqPacket();
    ASSERT_EQ(signalId1, 1u);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignalEventPacket, packet1);

    auto [signalId2, packet2] = client.getNextDaqPacket();
    ASSERT_EQ(signalId2, 2u);
    ASSERT_EQ(serverDataDescriptorChangedOnDomainSignalEventPacket, packet2);

    auto [signalId3, packet3] = client.getNextDaqPacket();
    ASSERT_EQ(signalId3, 2u);
    ASSERT_EQ(serverDomainDataPacket, packet3);

    auto [signalId4, packet4] = client.getNextDaqPacket();
    ASSERT_EQ(signalId4, 1u);
    //    ASSERT_EQ(serverDomainDataPacket, packet4);

    serverDomainDataPacket.release();

    completeTransmitAll();
    ASSERT_TRUE(client.areReferencesCleared());
}

TEST_F(PacketStreamingTest, DomainPacketTwice)
{
    constexpr uint32_t valueSignal1 = 1;
    constexpr uint32_t domainSignal1 = 2;
    constexpr uint32_t valueSignal2 = 3;
    constexpr uint32_t domainSignal2 = 4;

    const auto valueDescriptor1 = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto valueDescriptor2 = DataDescriptorBuilder().setSampleType(SampleType::Int16).build();

    const auto domainDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(LinearDataRule(1, 0)).setTickResolution(Ratio(1, 1000)).build();

    const auto serverDataDescriptorChangedOnValueSignal1EventPacket = DataDescriptorChangedEventPacket(valueDescriptor1, domainDescriptor);
    server.addDaqPacket(valueSignal1, serverDataDescriptorChangedOnValueSignal1EventPacket);

    const auto serverDataDescriptorChangedOnValueSignal2EventPacket = DataDescriptorChangedEventPacket(valueDescriptor2, domainDescriptor);
    server.addDaqPacket(valueSignal2, serverDataDescriptorChangedOnValueSignal2EventPacket);

    const auto serverDataDescriptorChangedOnDomainSignal1EventPacket = DataDescriptorChangedEventPacket(domainDescriptor, nullptr);
    server.addDaqPacket(domainSignal1, serverDataDescriptorChangedOnDomainSignal1EventPacket);

    const auto serverDataDescriptorChangedOnDomainSignal2EventPacket = DataDescriptorChangedEventPacket(domainDescriptor, nullptr);
    server.addDaqPacket(domainSignal2, serverDataDescriptorChangedOnDomainSignal2EventPacket);

    constexpr size_t sampleCount = 100;

    auto serverDomainDataPacket = DataPacket(domainDescriptor, sampleCount, 1024);

    auto serverValueDataPacket1 = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor1, sampleCount);
    auto data1 = static_cast<float*>(serverValueDataPacket1.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data1++ = static_cast<float>(i);

    auto serverValueDataPacket2 = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor2, sampleCount);
    auto data2 = static_cast<int16_t*>(serverValueDataPacket2.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data2++ = static_cast<int16_t>(i);

    server.addDaqPacket(valueSignal1, serverValueDataPacket1);
    server.addDaqPacket(valueSignal2, serverValueDataPacket2);
    server.addDaqPacket(domainSignal1, serverDomainDataPacket);
    server.addDaqPacket(domainSignal2, serverDomainDataPacket);

    transmitAll();

    auto [signalId1, packet1] = client.getNextDaqPacket();
    ASSERT_EQ(signalId1, valueSignal1);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignal1EventPacket, packet1);

    auto [signalId2, packet2] = client.getNextDaqPacket();
    ASSERT_EQ(signalId2, valueSignal2);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignal2EventPacket, packet2);

    auto [signalId3, packet3] = client.getNextDaqPacket();
    ASSERT_EQ(signalId3, domainSignal1);
    ASSERT_EQ(serverDataDescriptorChangedOnDomainSignal1EventPacket, packet3);

    auto [signalId4, packet4] = client.getNextDaqPacket();
    ASSERT_EQ(signalId4, domainSignal2);
    ASSERT_EQ(serverDataDescriptorChangedOnDomainSignal2EventPacket, packet4);

    auto [signalId5, packet5] = client.getNextDaqPacket();
    bool packet5destroyed = false;
    packet5.subscribeForDestructNotification(PacketDestructCallback([&packet5destroyed] { packet5destroyed = true; }));

    ASSERT_EQ(signalId5, domainSignal1);
    ASSERT_EQ(serverDomainDataPacket, packet5);

    auto [signalId6, packet6] = client.getNextDaqPacket();
    bool packet6destroyed = false;
    packet6.subscribeForDestructNotification(PacketDestructCallback([&packet6destroyed] { packet6destroyed = true; }));

    ASSERT_EQ(signalId6, valueSignal1);
    ASSERT_EQ(serverValueDataPacket1, packet6);

    auto [signalId7, packet7] = client.getNextDaqPacket();
    bool packet7destroyed = false;
    packet7.subscribeForDestructNotification(PacketDestructCallback([&packet7destroyed] { packet7destroyed = true; }));

    ASSERT_EQ(signalId7, valueSignal2);
    ASSERT_EQ(serverValueDataPacket2, packet7);

    auto [signalId8, packet8] = client.getNextDaqPacket();
    bool packet8destroyed = false;
    packet8.subscribeForDestructNotification(PacketDestructCallback([&packet8destroyed] { packet8destroyed = true; }));

    ASSERT_EQ(signalId8, domainSignal2);
    ASSERT_EQ(serverDomainDataPacket, packet8);

    auto [signalId9, packet9] = client.getNextDaqPacket();
    ASSERT_EQ(signalId9, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet9);

    serverDomainDataPacket.release();
    serverValueDataPacket1.release();
    serverValueDataPacket2.release();

    packet5.release();
    ASSERT_FALSE(packet5destroyed);

    packet6.release();
    ASSERT_FALSE(packet6destroyed);

    packet7.release();
    ASSERT_FALSE(packet7destroyed);

    packet8.release();
    ASSERT_FALSE(packet8destroyed);

    server.checkAndSendReleasePacket(true);

    transmitAll();

    auto [signalId10, packet10] = client.getNextDaqPacket();
    ASSERT_EQ(signalId10, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet10);

    ASSERT_TRUE(packet5destroyed);
    ASSERT_TRUE(packet6destroyed);
    ASSERT_TRUE(packet7destroyed);
    ASSERT_TRUE(packet8destroyed);
}

TEST_F(PacketStreamingTest, DomainPacketTwiceRelease)
{
    constexpr uint32_t valueSignal1 = 1;
    constexpr uint32_t domainSignal1 = 2;
    constexpr uint32_t valueSignal2 = 3;
    constexpr uint32_t domainSignal2 = 4;

    const auto valueDescriptor1 = DataDescriptorBuilder().setSampleType(SampleType::Float32).build();

    const auto valueDescriptor2 = DataDescriptorBuilder().setSampleType(SampleType::Int16).build();

    const auto domainDescriptor =
        DataDescriptorBuilder().setSampleType(SampleType::Int64).setRule(LinearDataRule(1, 0)).setTickResolution(Ratio(1, 1000)).build();

    const auto serverDataDescriptorChangedOnValueSignal1EventPacket = DataDescriptorChangedEventPacket(valueDescriptor1, domainDescriptor);
    server.addDaqPacket(valueSignal1, serverDataDescriptorChangedOnValueSignal1EventPacket);

    const auto serverDataDescriptorChangedOnValueSignal2EventPacket = DataDescriptorChangedEventPacket(valueDescriptor2, domainDescriptor);
    server.addDaqPacket(valueSignal2, serverDataDescriptorChangedOnValueSignal2EventPacket);

    const auto serverDataDescriptorChangedOnDomainSignal1EventPacket = DataDescriptorChangedEventPacket(domainDescriptor, nullptr);
    server.addDaqPacket(domainSignal1, serverDataDescriptorChangedOnDomainSignal1EventPacket);

    const auto serverDataDescriptorChangedOnDomainSignal2EventPacket = DataDescriptorChangedEventPacket(domainDescriptor, nullptr);
    server.addDaqPacket(domainSignal2, serverDataDescriptorChangedOnDomainSignal2EventPacket);

    constexpr size_t sampleCount = 100;

    auto serverDomainDataPacket = DataPacket(domainDescriptor, sampleCount, 1024);

    auto serverValueDataPacket1 = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor1, sampleCount);
    auto data1 = static_cast<float*>(serverValueDataPacket1.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data1++ = static_cast<float>(i);

    auto serverValueDataPacket2 = DataPacketWithDomain(serverDomainDataPacket, valueDescriptor2, sampleCount);
    auto data2 = static_cast<int16_t*>(serverValueDataPacket2.getRawData());
    for (size_t i = 0; i < sampleCount; i++)
        *data2++ = static_cast<int16_t>(i);

    server.addDaqPacket(valueSignal1, std::move(serverValueDataPacket1));
    transmitAll();
    server.addDaqPacket(valueSignal2, std::move(serverValueDataPacket2));
    transmitAll();
    server.addDaqPacket(domainSignal1, serverDomainDataPacket);
    transmitAll();
    server.addDaqPacket(domainSignal2, std::move(serverDomainDataPacket));
    transmitAll();

    auto [signalId1, packet1] = client.getNextDaqPacket();
    ASSERT_EQ(signalId1, valueSignal1);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignal1EventPacket, packet1);

    auto [signalId2, packet2] = client.getNextDaqPacket();
    ASSERT_EQ(signalId2, valueSignal2);
    ASSERT_EQ(serverDataDescriptorChangedOnValueSignal2EventPacket, packet2);

    auto [signalId3, packet3] = client.getNextDaqPacket();
    ASSERT_EQ(signalId3, domainSignal1);
    ASSERT_EQ(serverDataDescriptorChangedOnDomainSignal1EventPacket, packet3);

    auto [signalId4, packet4] = client.getNextDaqPacket();
    ASSERT_EQ(signalId4, domainSignal2);
    ASSERT_EQ(serverDataDescriptorChangedOnDomainSignal2EventPacket, packet4);

    auto [signalId5, packet5] = client.getNextDaqPacket();
    bool packet5destroyed = false;
    packet5.subscribeForDestructNotification(PacketDestructCallback([&packet5destroyed] { packet5destroyed = true; }));

    ASSERT_EQ(signalId5, domainSignal1);

    auto [signalId6, packet6] = client.getNextDaqPacket();
    bool packet6destroyed = false;
    packet6.subscribeForDestructNotification(PacketDestructCallback([&packet6destroyed] { packet6destroyed = true; }));

    ASSERT_EQ(signalId6, valueSignal1);

    auto [signalId7, packet7] = client.getNextDaqPacket();
    bool packet7destroyed = false;
    packet7.subscribeForDestructNotification(PacketDestructCallback([&packet7destroyed] { packet7destroyed = true; }));

    ASSERT_EQ(signalId7, valueSignal2);

    auto [signalId8, packet8] = client.getNextDaqPacket();
    bool packet8destroyed = false;
    packet8.subscribeForDestructNotification(PacketDestructCallback([&packet8destroyed] { packet8destroyed = true; }));

    ASSERT_EQ(signalId8, domainSignal2);

    auto [signalId9, packet9] = client.getNextDaqPacket();
    ASSERT_EQ(signalId9, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet9);

    packet5.release();
    packet6.release();
    ASSERT_TRUE(packet6destroyed);
    ASSERT_FALSE(packet5destroyed);

    packet7.release();
    ASSERT_TRUE(packet7destroyed);
    packet8.release();
    ASSERT_TRUE(packet8destroyed);
    ASSERT_TRUE(packet5destroyed);

    server.checkAndSendReleasePacket(true);

    transmitAll();

    auto [signalId10, packet10] = client.getNextDaqPacket();
    ASSERT_EQ(signalId10, std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(nullptr, packet10);
}

INSTANTIATE_TEST_SUITE_P(MovePacket, ValuePacketDestroyedBeforeDomainSentTest, testing::Values(true, false));
