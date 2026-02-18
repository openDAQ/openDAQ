#include <opendaq/module_manager_factory.h>
#include <testutils/testutils.h>
#include <testutils/daq_memcheck_listener.h>

int main(int argc, char** args)
{
    {
        daq::ModuleManager(".");
    }
    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    return RUN_ALL_TESTS();
}
