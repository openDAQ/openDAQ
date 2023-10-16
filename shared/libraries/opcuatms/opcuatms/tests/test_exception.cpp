#include <gtest/gtest.h>
#include <opcuatms/exceptions.h>

using ExceptionTest = testing::Test;

using namespace daq;
using namespace daq::opcua;

TEST_F(ExceptionTest, ThrowException)
{
    EXPECT_THROW(throw OpcUaGeneralException(), OpcUaGeneralException);
    EXPECT_THROW(throwExceptionFromErrorCode(OPENDAQ_ERR_OPCUA_GENERAL), OpcUaGeneralException);
}
