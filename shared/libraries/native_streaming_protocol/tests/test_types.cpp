#include <gtest/gtest.h>
#include <testutils/memcheck_listener.h>
#include <native_streaming_protocol/native_streaming_protocol_types.h>
#include <memory>

using namespace daq;
using namespace daq::opendaq_native_streaming_protocol;

using TypesTest = testing::Test;

TEST_F(TypesTest, TransportHeader)
{
    const size_t payloadSize = 123;

    for (const auto payloadType : allPayloadTypes)
    {
        const auto header1 = TransportHeader(payloadType, payloadSize);
        ASSERT_EQ(header1.getPayloadType(), payloadType);
        ASSERT_EQ(header1.getPayloadSize(), payloadSize);

        auto header2 = TransportHeader(header1.getPackedHeaderPtr());
        ASSERT_EQ(header2.getPayloadType(), payloadType);
        ASSERT_EQ(header2.getPayloadSize(), payloadSize);
    }
}
