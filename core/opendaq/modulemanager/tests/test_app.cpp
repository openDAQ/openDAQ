#include <testutils/testutils.h>
#include <testutils/bb_memcheck_listener.h>
#include <coretypes/stringobject_factory.h>
#include <opendaq/module_manager_init.h>

int main(int argc, char** args)
{
    daqInitModuleManagerLibrary();

    testing::InitGoogleTest(&argc, args);

    auto& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    auto res = RUN_ALL_TESTS();

    return res;
}
