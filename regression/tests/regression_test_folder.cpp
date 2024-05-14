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
    ListPtr<IComponent> items;
    ASSERT_NO_THROW(items = folder.getItems());
    ASSERT_GT(items.getCount(), 0);
}

TEST_P(RegressionTestFolder, isEmpty)
{
    Bool isEmpty;
    ASSERT_NO_THROW(isEmpty = folder.isEmpty());
    ASSERT_FALSE(isEmpty);
}

TEST_P(RegressionTestFolder, hasItem)
{
    Bool hasItem;
    ASSERT_NO_THROW(hasItem = folder.hasItem("Sig"));
    ASSERT_TRUE(hasItem);
}

TEST_P(RegressionTestFolder, getItem)
{
    ComponentPtr item;
    ASSERT_NO_THROW(item = folder.getItem("Sig"));
    ASSERT_TRUE(item.assigned());
}

INSTANTIATE_TEST_SUITE_P(Folder,
                         RegressionTestFolder,
                         testing::Values("daq.opcua://127.0.0.1", "daq.nd://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
