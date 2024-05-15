#include <gtest/gtest.h>
#include "get_protocol.h"

using namespace daq;

class RegressionTestFolder : public testing::Test
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

        folder = instance.addDevice(connectionString);
    }
};

TEST_F(RegressionTestFolder, getItems)
{
    ListPtr<IComponent> items;
    ASSERT_NO_THROW(items = folder.getItems());
    ASSERT_GT(items.getCount(), 0);
}

TEST_F(RegressionTestFolder, isEmpty)
{
    Bool isEmpty;
    ASSERT_NO_THROW(isEmpty = folder.isEmpty());
    ASSERT_FALSE(isEmpty);
}

TEST_F(RegressionTestFolder, hasItem)
{
    Bool hasItem;
    ASSERT_NO_THROW(hasItem = folder.hasItem("Sig"));
    ASSERT_TRUE(hasItem);
}

TEST_F(RegressionTestFolder, getItem)
{
    ComponentPtr item;
    ASSERT_NO_THROW(item = folder.getItem("Sig"));
    ASSERT_TRUE(item.assigned());
}
