#include <coretypes/stringobject_factory.h>
#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>

int main(int argc, char** args)
{
    using namespace daq;

    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return res;
}
