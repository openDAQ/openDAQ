#include "setup_regression.h"

class RegressionTestComponent : public testing::Test
{
private:
    ModuleManagerPtr moduleManager;
    ContextPtr context;
    InstancePtr instance;

protected:
    ComponentPtr component;

    void SetUp() override
    {
        moduleManager = ModuleManager("");
        context = Context(nullptr, Logger(), TypeManager(), moduleManager);

        instance = InstanceCustom(context, "mock_instance");

        component = instance.addDevice(connectionString);
    }
};

TEST_F(RegressionTestComponent, getLocalId)
{
    StringPtr id;
    ASSERT_NO_THROW(id = component.getLocalId());
    StringPtr realId;
    if (protocol == "opcua" || protocol == "nd")
        realId = "ref_dev1";
    else if (protocol == "ns")
        realId = "streaming_pseudo_device0";
    else if (protocol == "lt")
        realId = "websocket_pseudo_device0";
    ASSERT_EQ(id, realId);
}

TEST_F(RegressionTestComponent, getGlobalId)
{
    StringPtr id;
    ASSERT_NO_THROW(id = component.getGlobalId());
    StringPtr realId;
    if (protocol == "opcua" || protocol == "nd")
        realId = "/mock_instance/Dev/ref_dev1";
    else if (protocol == "ns")
        realId = "/mock_instance/Dev/streaming_pseudo_device0";
    else if (protocol == "lt")
        realId = "/mock_instance/Dev/websocket_pseudo_device0";
    ASSERT_EQ(id, realId);
}

TEST_F(RegressionTestComponent, setActiveGetActive)
{
    ASSERT_NO_THROW(component.setActive(True));
    Bool active;
    ASSERT_NO_THROW(active = component.getActive());
    ASSERT_TRUE(active);
}

TEST_F(RegressionTestComponent, setNameGetName)
{
    StringPtr newName = "test_name";
    ASSERT_NO_THROW(component.setName(newName));
    StringPtr name;
    ASSERT_NO_THROW(name = component.getName());
    ASSERT_EQ(name, newName);
}

TEST_F(RegressionTestComponent, setDescriptionGetDescription)
{
    StringPtr newDescription = "test_description";
    ASSERT_NO_THROW(component.setDescription(newDescription));
    StringPtr description;
    ASSERT_NO_THROW(description = component.getDescription());
    ASSERT_EQ(description, newDescription);
}

TEST_F(RegressionTestComponent, getTags)
{
    TagsPtr tags;
    ASSERT_NO_THROW(tags = component.getTags());
    ASSERT_TRUE(tags.assigned());
}

TEST_F(RegressionTestComponent, setVisibleGetVisible)
{
    auto componentPrivate = component.asPtr<IComponentPrivate>();
    componentPrivate.unlockAllAttributes();

    ASSERT_NO_THROW(component.setVisible(False));
    Bool visible1;
    ASSERT_NO_THROW(visible1 = component.getVisible());
    ASSERT_FALSE(visible1);

    ASSERT_NO_THROW(component.setVisible(True));
    Bool visible2;
    ASSERT_NO_THROW(visible2 = component.getVisible());
    ASSERT_TRUE(visible2);
}

TEST_F(RegressionTestComponent, getLockedAttributes)
{
    ListPtr<IString> lockedAttributes;
    ASSERT_NO_THROW(lockedAttributes = component.getLockedAttributes());
    ASSERT_EQ(lockedAttributes.getCount(), 1);
}

TEST_F(RegressionTestComponent, getStatusContainer)
{
    ComponentStatusContainerPtr container;
    ASSERT_NO_THROW(container = component.getStatusContainer());
    ASSERT_TRUE(container.assigned());
}
