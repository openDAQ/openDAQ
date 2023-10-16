#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>
#include <coreobjects/util.h>

int main(int argc, char** args)
{
    daq::daqInitializeCoreObjectsTesting();

    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    return RUN_ALL_TESTS();
}
