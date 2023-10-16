#include <gtest/gtest.h>
#include <testutils/memcheck_listener.h> 

int main(int argc, char** args)
{
    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new MemCheckListener());

    return RUN_ALL_TESTS();
}
