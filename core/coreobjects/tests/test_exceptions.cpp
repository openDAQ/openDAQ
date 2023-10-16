#include <gtest/gtest.h>
#include <coreobjects/exceptions.h>

using ExceptionsTest = testing::Test;

using namespace daq;

TEST_F(ExceptionsTest, Exception)
{
    // CoreObjects exception
    ASSERT_THROW(throwExceptionFromErrorCode(OPENDAQ_ERR_CALCFAILED), CalcFailedException);
}

TEST_F(ExceptionsTest, Exception2)
{
    // CoreTypes exception
    ASSERT_THROW(throwExceptionFromErrorCode(OPENDAQ_ERR_NOMEMORY), NoMemoryException);
}
