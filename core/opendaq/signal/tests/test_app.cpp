#include <coretypes/stringobject_factory.h>
#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>
#include <opendaq/module_manager_init.h>

int main(int argc, char** args)
{
    using namespace daq;

    testing::InitGoogleTest(&argc, args);
    daqInitModuleManagerLibrary();

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return res;
}
