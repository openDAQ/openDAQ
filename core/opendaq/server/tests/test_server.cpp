#include <gtest/gtest.h>
#include "mock/mock_server.h"
#include <opendaq/server_ptr.h>

using ServerTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(ServerTest, ServerFactory)
{
    ServerPtr server;
    auto err = createMockServer(&server);
    ASSERT_EQ(err, OPENDAQ_SUCCESS);
}


END_NAMESPACE_OPENDAQ
