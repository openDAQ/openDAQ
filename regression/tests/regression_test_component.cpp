#include <gtest/gtest.h>
#include <opendaq/mock/mock_device_module.h>
#include <opendaq/opendaq.h>

using namespace daq;

class RegressionTestComponent : public testing::TestWithParam<StringPtr>
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

        component = instance.addDevice(GetParam());
    }
};

TEST_P(RegressionTestComponent, getLocalId)
{
    StringPtr id;
    ASSERT_NO_THROW(id = component.getLocalId());
    StringPtr realId;
    auto connectionString = GetParam();
    if (connectionString == "daq.opcua://127.0.0.1")
        realId = "ref_dev1";
    else if (connectionString == "daq.ns://127.0.0.1")
        realId = "streaming_pseudo_device0";
    else if (connectionString == "daq.lt://127.0.0.1")
        realId = "websocket_pseudo_device0";
    ASSERT_EQ(id, realId);
}

TEST_P(RegressionTestComponent, getGlobalId)
{
    StringPtr id;
    ASSERT_NO_THROW(id = component.getGlobalId());
    StringPtr realId;
    auto connectionString = GetParam();
    if (connectionString == "daq.opcua://127.0.0.1")
        realId = "/mock_instance/Dev/ref_dev1";
    else if (connectionString == "daq.ns://127.0.0.1")
        realId = "/mock_instance/Dev/streaming_pseudo_device0";
    else if (connectionString == "daq.lt://127.0.0.1")
        realId = "/mock_instance/Dev/websocket_pseudo_device0";
    ASSERT_EQ(id, realId);
}

TEST_P(RegressionTestComponent, setActiveGetActive)
{
    ASSERT_NO_THROW(component.setActive(True));
    Bool active;
    ASSERT_NO_THROW(active = component.getActive());
    ASSERT_TRUE(active);
}

TEST_P(RegressionTestComponent, getContext)
{
    ContextPtr context;
    ASSERT_NO_THROW(context = component.getContext());
    ASSERT_TRUE(context.assigned());
}

TEST_P(RegressionTestComponent, getParent)
{
    ComponentPtr parent;
    ASSERT_NO_THROW(parent = component.getParent());
    ASSERT_EQ(parent.getName(), "Dev");
}

TEST_P(RegressionTestComponent, setNameGetName)
{
    StringPtr newName = "test_name";
    ASSERT_NO_THROW(component.setName(newName));
    StringPtr name;
    ASSERT_NO_THROW(name = component.getName());
    ASSERT_EQ(name, newName);
}

TEST_P(RegressionTestComponent, setDescriptionGetDescription)
{
    StringPtr newDescription = "test_description";
    ASSERT_NO_THROW(component.setDescription(newDescription));
    StringPtr description;
    ASSERT_NO_THROW(description = component.getDescription());
    ASSERT_EQ(description, newDescription);
}

TEST_P(RegressionTestComponent, getTags)
{
    TagsPtr tags;
    ASSERT_NO_THROW(tags = component.getTags());
    ASSERT_TRUE(tags.assigned());
}

TEST_P(RegressionTestComponent, setVisibleGetVisible)
{
    ASSERT_NO_THROW(component.setVisible(True));  // TODO: does nothing, because the attribute is locked
    Bool visible;
    ASSERT_NO_THROW(visible = component.getVisible());
    ASSERT_TRUE(visible);
}

TEST_P(RegressionTestComponent, getLockedAttributes)
{
    ListPtr<IString> lockedAttributes;
    ASSERT_NO_THROW(lockedAttributes = component.getLockedAttributes());
    ASSERT_EQ(lockedAttributes.getCount(), 1);
}

TEST_P(RegressionTestComponent, getOnComponentCoreEvent)
{
    auto event = component.getOnComponentCoreEvent();
    ASSERT_NE(event, nullptr);
}

TEST_P(RegressionTestComponent, getStatusContainer)
{
    ComponentStatusContainerPtr container;
    ASSERT_NO_THROW(container = component.getStatusContainer());
    ASSERT_TRUE(container.assigned());
}

TEST_P(RegressionTestComponent, findComponent)
{
    ComponentPtr comp;
    ASSERT_NO_THROW(comp = component.findComponent("Sig"));
    ASSERT_EQ(comp.getName(), "Sig");
}

INSTANTIATE_TEST_SUITE_P(Component,
                         RegressionTestComponent,
                         testing::Values("daq.opcua://127.0.0.1", "daq.nd://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
