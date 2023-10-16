#include <opendaq/packet_destruct_callback_factory.h>
#include <gtest/gtest.h>

using PacketDestructCallbackTest = testing::Test;

TEST_F(PacketDestructCallbackTest, PacketDestruct)
{
    bool called = false;
    const auto packetDestructCallback = daq::PacketDestructCallback(
        [&called]() {
            called = true;
        });

    packetDestructCallback.onPacketDestroyed();
    ASSERT_TRUE(called);
}
