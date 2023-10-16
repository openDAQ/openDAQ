#include <gtest/gtest.h>
#include <opendaq/utils/finally.h>

using FinallyTest = testing::Test;

using namespace daq::utils;
using namespace testing;

TEST_F(FinallyTest, CallDestructor)
{
    int* test = new int(0);
    Finally f([test] { delete test; });
}
