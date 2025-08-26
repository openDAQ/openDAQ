#include <opendaq/module_manager_init.h>
#include <opendaq/opendaq_init.h>
#include <coreobjects/util.h>
#include <testutils/bb_memcheck_listener.h>

int main(int argc, char** args)
{
    daq::daqInitializeCoreObjectsTesting();
    daqInitModuleManagerLibrary();
    daqInitOpenDaqLibrary();

    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return res;
}
