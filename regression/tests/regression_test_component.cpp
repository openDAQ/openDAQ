#include <gtest/gtest.h>
#include <opendaq/mock/mock_device_module.h>
#include "get_protocol.h"

using namespace daq;

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
    if (connectionString == "daq.opcua://127.0.0.1")
        realId = "ref_dev1";
    else if (connectionString == "daq.ns://127.0.0.1")
        realId = "streaming_pseudo_device0";
    else if (connectionString == "daq.lt://127.0.0.1")
        realId = "websocket_pseudo_device0";
    ASSERT_EQ(id, realId);
}

TEST_F(RegressionTestComponent, getGlobalId)
{
    StringPtr id;
    ASSERT_NO_THROW(id = component.getGlobalId());
    StringPtr realId;
    if (connectionString == "daq.opcua://127.0.0.1")
        realId = "/mock_instance/Dev/ref_dev1";
    else if (connectionString == "daq.ns://127.0.0.1")
        realId = "/mock_instance/Dev/streaming_pseudo_device0";
    else if (connectionString == "daq.lt://127.0.0.1")
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
    ASSERT_NO_THROW(component.setVisible(True));  // TODO: does nothing, because the attribute is locked
    Bool visible;
    ASSERT_NO_THROW(visible = component.getVisible());
    ASSERT_TRUE(visible);
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
