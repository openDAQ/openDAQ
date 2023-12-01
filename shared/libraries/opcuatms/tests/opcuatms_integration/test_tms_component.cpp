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

    RegisteredComponent registerTestComponent(const ComponentPtr& customComponent = nullptr)
    {
        RegisteredComponent component{};

        if (customComponent == nullptr)
            component.serverComponent = createTestComponent();
        else
            component.serverComponent = customComponent;
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

TEST_F(TmsComponentTest, NameAndDescription)
{
    const auto component = registerTestComponent();
    ASSERT_EQ(component.serverComponent.getName(), component.clientComponent.getName());
    ASSERT_EQ(component.serverComponent.getDescription(), component.clientComponent.getDescription());

    component.serverComponent.setName("new_name");
    component.serverComponent.setDescription("new_description");

    ASSERT_EQ(component.serverComponent.getName(), component.clientComponent.getName());
    ASSERT_EQ(component.serverComponent.getDescription(), component.clientComponent.getDescription());

    component.clientComponent.setName("newer_name");
    component.clientComponent.setDescription("newer_description");

    ASSERT_EQ(component.serverComponent.getName(), component.clientComponent.getName());
    ASSERT_EQ(component.serverComponent.getDescription(), component.clientComponent.getDescription());
}

TEST_F(TmsComponentTest, NameAndDescriptionReadOnly)
{
    const auto name = "read_only";
    const auto customComponent = Component(NullContext(), nullptr, name, ComponentStandardProps::AddReadOnly);
    const auto component = registerTestComponent(customComponent);

    ASSERT_NO_THROW(component.clientComponent.setName("new_name"));
    ASSERT_NO_THROW(component.clientComponent.setDescription("new_description"));

    ASSERT_EQ(component.clientComponent.getName(), name);
}

TEST_F(TmsComponentTest, NameAndDescriptionSkip)
{
    const auto name = "read_only";
    const auto customComponent = Component(NullContext(), nullptr, name, ComponentStandardProps::Skip);
    const auto component = registerTestComponent(customComponent);

    ASSERT_NO_THROW(component.clientComponent.setName("new_name"));
    ASSERT_NO_THROW(component.clientComponent.setDescription("new_description"));

    ASSERT_EQ(component.clientComponent.getName(), name);
}