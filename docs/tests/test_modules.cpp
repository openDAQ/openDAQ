#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using ModulesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, ModuleManager)
{
    daq::ModuleManagerPtr manager1 = daq::ModuleManager("");
    ASSERT_NO_THROW(manager1.loadModules(NullContext()));
    daq::ModuleManagerPtr manager2 = daq::ModuleManager(".");
    ASSERT_NO_THROW(manager2.loadModules(NullContext()));
    daq::ModuleManagerPtr manager3 = daq::ModuleManager("./dir1/dir2");
    ASSERT_NO_THROW(manager3.loadModules(NullContext()));
    daq::ModuleManagerPtr manager4 = daq::ModuleManager("/invalid_dir/dir1/dir2");
    ASSERT_NO_THROW(manager4.loadModules(NullContext()));
    daq::ModuleManagerPtr manager5 = daq::ModuleManager("[[none]]");
    ASSERT_NO_THROW(manager5.loadModules(NullContext()));
}

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, BBInstanceModules)
{
    ASSERT_NO_THROW(daq::InstancePtr instance1 = daq::Instance());
    ASSERT_NO_THROW(daq::InstancePtr instance2 = daq::Instance("."));
}

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, EnumerateModules)
{
    // Create the module manager and load modules in the executable directory
    daq::ModuleManagerPtr manager = daq::ModuleManager("");
    manager.loadModules(NullContext());
    ASSERT_GT(manager.getModules().getCount(), 0u);

    ModulePtr _module;
    for (auto mod : manager.getModules())
    {
        if (mod.getModuleInfo().getName() == "ReferenceFunctionBlockModule")
        {
            _module = mod;
            break;
        }
    }

    ASSERT_TRUE(_module.assigned());

    ASSERT_NO_THROW(const auto devices = _module.getAvailableDevices());
    ASSERT_NO_THROW(const auto functionBlockTypes = _module.getAvailableFunctionBlockTypes());
    ASSERT_NO_THROW(const auto serverTypes = _module.getAvailableServerTypes());
    ASSERT_NO_THROW(const auto deviceTypes = _module.getAvailableDeviceTypes());
}

// Corresponding document: Antora/modules/explanation/pages/modules.adoc
TEST_F(ModulesTest, InstanceModules)
{
    // Create the instance and load modules
    daq::InstancePtr instance = Instance();

    // List available function blocks
    daq::DictPtr<daq::IString, daq::IFunctionBlockType> functionBlockTypes = instance.getAvailableFunctionBlockTypes();

    // Add the statistics function block, if available
    if (functionBlockTypes.hasKey("RefFBModuleStatistics"))
        daq::FunctionBlockPtr functionBlock = instance.addFunctionBlock("RefFBModuleStatistics");
}

END_NAMESPACE_OPENDAQ
