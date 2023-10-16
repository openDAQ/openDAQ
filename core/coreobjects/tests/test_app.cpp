#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>
#include <coreobjects/coreobjects.h>
#include <coreobjects/util.h>

int main(int argc, char** args)
{
    using namespace daq;

    daqInitializeCoreObjectsTesting();

    ::testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    int res = RUN_ALL_TESTS();
    return res;
}
