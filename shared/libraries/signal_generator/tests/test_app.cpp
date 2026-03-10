#include <gtest/gtest.h>
#include <testutils/base_test_listener.h>

int main(int argc, char** args)
{
    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new BaseTestListener());

    return RUN_ALL_TESTS();
}
