#include <gtest/gtest.h>
#include "opcuashared/opcuaendpoint.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaEndpointTest = testing::Test;

TEST_F(OpcUaEndpointTest, CreateEmptyEndpoint)
{
    ASSERT_NO_THROW(OpcUaEndpoint ep{});
}

TEST_F(OpcUaEndpointTest, CreateEndpoint)
{
    OpcUaEndpoint ep{"name", "opc.tcp://localhost:4840"};
    ASSERT_EQ(ep.getName(), "name");
    ASSERT_EQ(ep.getUrl(), "opc.tcp://localhost:4840");
}

END_NAMESPACE_OPENDAQ_OPCUA
