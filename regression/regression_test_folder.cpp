#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestFolder : public testing::TestWithParam<StringPtr>
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;

protected:
    FolderPtr folder;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        folder = instance.addDevice(GetParam());
    }
};

TEST_P(RegressionTestFolder, getItems)
{
    ASSERT_NO_THROW(folder.getItems());
}

TEST_P(RegressionTestFolder, isEmpty)
{
    ASSERT_NO_THROW(folder.isEmpty());
}

TEST_P(RegressionTestFolder, hasItem)
{
    ASSERT_NO_THROW(folder.hasItem("test"));
}

TEST_P(RegressionTestFolder, getItem)
{
    ASSERT_NO_THROW(folder.getItem("Sig"));
}

INSTANTIATE_TEST_SUITE_P(Folder,
                         RegressionTestFolder,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
