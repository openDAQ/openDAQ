#include <gtest/gtest.h>
#include <testutils/daq_memcheck_listener.h>

int main(int argc, char** args)
{
    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    return RUN_ALL_TESTS();
}
