#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/config_protocol.h>

using namespace daq;
using namespace config_protocol;

using ConfigPacketTest = testing::Test;

TEST_F(ConfigPacketTest, PacketBufferMove)
{
    PacketBuffer packetBufferSource(PacketType::getProtocolInfo, 1, nullptr, 0);
    PacketBuffer packetBuffer(std::move(packetBufferSource));
}

TEST_F(ConfigPacketTest, PacketBufferMoveWithDeleter)
{
    PacketBuffer packetBufferSource(PacketType::getProtocolInfo, 1, nullptr, 0);
    PacketBuffer packetBufferSource1(packetBufferSource.detach(), [](void* ptr) { PacketBuffer::deallocateMem(ptr); });
    PacketBuffer packetBuffer(std::move(packetBufferSource1));
}

TEST_F(ConfigPacketTest, PacketBuffer)
{
    const std::vector<uint8_t> buf = {0, 1, 2, 3, 4, 5, 6, 7};

    const PacketBuffer packetBuffer(PacketType::getProtocolInfo, 1, buf.data(), 8);

    ASSERT_EQ(packetBuffer.getPacketType(), PacketType::getProtocolInfo);
    ASSERT_EQ(packetBuffer.getId(), 1);

    std::vector<uint8_t> buf1;
    buf1.resize(8);
    std::memcpy(buf1.data(), packetBuffer.getPayload(), 8);
    ASSERT_THAT(buf1, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7));
}

using ConfigPacketFromParamTest = testing::TestWithParam<bool>;

TEST_P(ConfigPacketFromParamTest, PacketBufferFromMemoryCopy)
{
    const std::vector<uint8_t> buf = {0, 1, 2, 3, 4, 5, 6, 7};

    const PacketBuffer packetBufferSource(PacketType::getProtocolInfo, 1, buf.data(), 8);

    const PacketBuffer packetBuffer(packetBufferSource.getBuffer(), GetParam());

    ASSERT_EQ(packetBuffer.getPacketType(), PacketType::getProtocolInfo);
    ASSERT_EQ(packetBuffer.getId(), 1);

    std::vector<uint8_t> buf1;
    buf1.resize(8);
    std::memcpy(buf1.data(), packetBuffer.getPayload(), 8);
    ASSERT_THAT(buf1, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7));
}

INSTANTIATE_TEST_SUITE_P(Instantiation,
                         ConfigPacketFromParamTest,
                         testing::Values(true, false),
                         [](const testing::TestParamInfo<ConfigPacketFromParamTest::ParamType>& info)
                         {
                             return info.param ? "copy" : "borrow";
                         });

TEST_F(ConfigPacketTest, PacketBufferWithDeleter)
{
    const std::vector<uint8_t> buf = {0, 1, 2, 3, 4, 5, 6, 7};

    PacketBuffer packetBufferSource(PacketType::getProtocolInfo, 1, buf.data(), 8);

    const auto mem = packetBufferSource.detach();

    const PacketBuffer packetBuffer(mem, [](void* ptr)
    {
        PacketBuffer::deallocateMem(ptr);
    });

    ASSERT_EQ(packetBuffer.getPacketType(), PacketType::getProtocolInfo);
    ASSERT_EQ(packetBuffer.getId(), 1);

    std::vector<uint8_t> buf1;
    buf1.resize(8);
    std::memcpy(buf1.data(), packetBuffer.getPayload(), 8);
    ASSERT_THAT(buf1, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7));
}

TEST_F(ConfigPacketTest, GetProtocolInfoRequest)
{
    const auto packetBufferSource = PacketBuffer::createGetProtocolInfoRequest(1);

    const PacketBuffer packetBuffer(packetBufferSource.getBuffer(), false);

    ASSERT_EQ(packetBuffer.getId(), 1);
    packetBuffer.parseProtocolInfoRequest();
}

TEST_F(ConfigPacketTest, GetProtocolInfoReply)
{
    const auto packetBufferSource = PacketBuffer::createGetProtocolInfoReply(1, 2, {0, 1, 2, 3});

    const PacketBuffer packetBuffer(packetBufferSource.getBuffer(), false);

    ASSERT_EQ(packetBuffer.getId(), 1);
    uint16_t curVer;
    std::vector<uint16_t> supVer;
    packetBuffer.parseProtocolInfoReply(curVer, supVer);

    ASSERT_EQ(curVer, 2);
    ASSERT_THAT(supVer, testing::ElementsAre(0, 1, 2, 3));
}

TEST_F(ConfigPacketTest, UpgradeProtocolRequest)
{
    const auto packetBufferSource = PacketBuffer::createUpgradeProtocolRequest(1, 2);

    const PacketBuffer packetBuffer(packetBufferSource.getBuffer(), false);

    ASSERT_EQ(packetBuffer.getId(), 1);
    uint16_t ver;
    packetBuffer.parseProtocolUpgradeRequest(ver);

    ASSERT_EQ(ver, 2);
}

TEST_F(ConfigPacketTest, UpgradeProtocolReply)
{
    const auto packetBufferSource = PacketBuffer::createUpgradeProtocolReply(1, true);

    const PacketBuffer packetBuffer(packetBufferSource.getBuffer(), false);

    ASSERT_EQ(packetBuffer.getId(), 1);
    bool success;
    packetBuffer.parseProtocolUpgradeReply(success);

    ASSERT_TRUE(success);
}

TEST_F(ConfigPacketTest, InvalidRequestReply)
{
    const auto packetBufferSource = PacketBuffer::createInvalidRequestReply(1);

    const PacketBuffer packetBuffer(packetBufferSource.getBuffer(), false);

    ASSERT_EQ(packetBuffer.getId(), 1);
    packetBuffer.parseInvalidRequestReply();
}

TEST_F(ConfigPacketTest, RpcRequest)
{
    const std::string json{"str"};
    const auto packetBufferSource = PacketBuffer::createRpcRequestOrReply(1, json.c_str(), json.length());

    const PacketBuffer packetBuffer(packetBufferSource.getBuffer(), false);

    ASSERT_EQ(packetBuffer.getId(), 1);
    const auto json1 = packetBuffer.parseRpcRequestOrReply();

    ASSERT_EQ(json1, json);
}

TEST_F(ConfigPacketTest, ServerNotification)
{
    const std::string json{"str"};
    const auto packetBufferSource = PacketBuffer::createServerNotification(json.c_str(), json.size());

    const PacketBuffer packetBuffer(packetBufferSource.getBuffer(), false);

    ASSERT_EQ(packetBuffer.getId(), std::numeric_limits<size_t>::max());
    const auto json1 = packetBuffer.parseServerNotification();

    ASSERT_EQ(json1, json);
}
