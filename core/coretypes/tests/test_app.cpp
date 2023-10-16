#include <gtest/gtest.h>
#include <testutils/bb_memcheck_listener.h>

int main(int argc, char** args)
{
    ::testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    int res = RUN_ALL_TESTS();
    return res;
}
