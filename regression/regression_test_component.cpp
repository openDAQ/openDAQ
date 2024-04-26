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
    DevicePtr component;

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
    ASSERT_NO_THROW(component.getLocalId());
}

TEST_P(RegressionTestComponent, getGlobalId)
{
    ASSERT_NO_THROW(component.getGlobalId());
}

TEST_P(RegressionTestComponent, getActive)
{
    ASSERT_NO_THROW(component.getActive());
}

TEST_P(RegressionTestComponent, setActive)
{
    ASSERT_NO_THROW(component.setActive(True));
}

TEST_P(RegressionTestComponent, getContext)
{
    ASSERT_NO_THROW(component.getContext());
}

TEST_P(RegressionTestComponent, getParent)
{
    ASSERT_NO_THROW(component.getParent());
}

TEST_P(RegressionTestComponent, getName)
{
    ASSERT_NO_THROW(component.getName());
}

TEST_P(RegressionTestComponent, setName)
{
    ASSERT_NO_THROW(component.setName("test_name"));
}

TEST_P(RegressionTestComponent, getDescription)
{
    ASSERT_NO_THROW(component.getDescription());
}

TEST_P(RegressionTestComponent, setDescription)
{
    ASSERT_NO_THROW(component.setDescription("test_description"));
}

TEST_P(RegressionTestComponent, getTags)
{
    ASSERT_NO_THROW(component.getTags());
}

TEST_P(RegressionTestComponent, getVisible)
{
    ASSERT_NO_THROW(component.getVisible());
}

TEST_P(RegressionTestComponent, setVisible)
{
    ASSERT_NO_THROW(component.setVisible(True));
}

TEST_P(RegressionTestComponent, getLockedAttributes)
{
    ASSERT_NO_THROW(component.getLockedAttributes());
}

TEST_P(RegressionTestComponent, getOnComponentCoreEvent)
{
    ASSERT_NO_THROW(component.getOnComponentCoreEvent());
}

TEST_P(RegressionTestComponent, getStatusContainer)
{
    ASSERT_NO_THROW(component.getStatusContainer());
}

TEST_P(RegressionTestComponent, findComponent)
{
    ASSERT_NO_THROW(component.findComponent("test_component"));
}

INSTANTIATE_TEST_SUITE_P(Component,
                         RegressionTestComponent,
                         testing::Values("daq.opcua://127.0.0.1", "daq.ns://127.0.0.1", "daq.lt://127.0.0.1"));
