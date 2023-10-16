#include "opendaq/component_factory.h"
#include "opcuatms_client/objects/tms_client_component_factory.h"
#include "opcuatms_server/objects/tms_server_component.h"
#include "tms_object_integration_test.h"
#include "coreobjects/property_object_factory.h"
#include "opendaq/context_factory.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

struct RegisteredComponent
{
    TmsServerComponentPtr serverObject;
    ComponentPtr serverComponent;
    ComponentPtr clientComponent;
};

class TmsComponentTest : public TmsObjectIntegrationTest
{
public:
    ComponentPtr createTestComponent()
    {
        auto component = Component(NullContext(), nullptr, "test");

        component.addProperty(StringProperty("foo", "bar"));
        auto obj = PropertyObject();
        obj.addProperty(IntProperty("int", 0));
        component.addProperty(ObjectProperty("obj", obj));

        component.getTags().add("tag1");
        component.getTags().add("tag2");

        return component;
    }

    RegisteredComponent registerTestComponent()
    {
        RegisteredComponent component{};
    
        component.serverComponent = createTestComponent();
        component.serverObject = std::make_shared<TmsServerComponent<>>(component.serverComponent, this->getServer(), NullContext());
        auto nodeId = component.serverObject->registerOpcUaNode();
        component.clientComponent = TmsClientComponent(NullContext(), nullptr, "test", clientContext, nodeId);
        return component;
    }
};

TEST_F(TmsComponentTest, Create)
{
    auto component = createTestComponent();
    auto serverComponent = TmsServerComponent(component, this->getServer(), NullContext());
}

TEST_F(TmsComponentTest, Register)
{
    auto component = registerTestComponent();
}

TEST_F(TmsComponentTest, Active)
{
    auto component = registerTestComponent();

    component.clientComponent.setActive(false);
    ASSERT_EQ(component.serverComponent.getActive(), component.clientComponent.getActive());

    component.clientComponent.setActive(true);
    ASSERT_EQ(component.serverComponent.getActive(), component.clientComponent.getActive());
}

TEST_F(TmsComponentTest, Tags)
{
    auto component = registerTestComponent();

    auto serverTags = component.serverComponent.getTags();
    auto clientTags = component.clientComponent.getTags();
    
    ASSERT_TRUE(clientTags.query("tag1") && clientTags.query("tag2"));
}

TEST_F(TmsComponentTest, Properties)
{
    auto component = registerTestComponent();

    PropertyObjectPtr serverObj = component.serverComponent.getPropertyValue("obj");
    PropertyObjectPtr clientObj = component.clientComponent.getPropertyValue("obj");
    ASSERT_EQ(serverObj.getPropertyValue("int"), clientObj.getPropertyValue("int"));
    ASSERT_EQ(component.serverComponent.getPropertyValue("foo"), component.clientComponent.getPropertyValue("foo"));

    component.clientComponent.setPropertyValue("foo", "notbar");
    ASSERT_EQ(component.serverComponent.getPropertyValue("foo"), component.clientComponent.getPropertyValue("foo"));
}
