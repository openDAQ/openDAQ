#include <array>
#include <opendaq/component_ptr.h>
#include <opendaq/component_impl.h>
#include <coretypes/objectptr.h>
#include <coretypes/type_manager_factory.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opendaq/gmock/context.h>
#include <coreobjects/property_object_class_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/component_exceptions.h>
#include <opendaq/component_impl.h>
#include <testutils/testutils.h>

using namespace daq;
using namespace testing;

class ComponentTest : public Test
{
protected:
    MockContext::Strict context;
};


TEST_F(ComponentTest, ID)
{
    auto parent = ComponentPtr::Adopt(Component_Create(context->getObject(), nullptr, StringPtr("parent"), nullptr));
    auto comp = ComponentPtr::Adopt(Component_Create(context->getObject(), parent, StringPtr("child"), nullptr));

    ASSERT_EQ(comp.getGlobalId(), "/parent/child");
    ASSERT_EQ(comp.getLocalId(), "child");
}

TEST_F(ComponentTest, Parent)
{
    auto parent = ComponentPtr::Adopt(Component_Create(context->getObject(), nullptr, StringPtr("parent"), nullptr));
    auto comp = ComponentPtr::Adopt(Component_Create(context->getObject(), parent, StringPtr("child"), nullptr));

    ASSERT_EQ(comp.getParent(), parent);
}

TEST_F(ComponentTest, Active)
{
    auto comp = ComponentPtr::Adopt(Component_Create(context->getObject(), nullptr, StringPtr("child"), nullptr));
    ASSERT_TRUE(comp.getActive());
    comp.setActive(false);
    ASSERT_FALSE(comp.getActive());
}

TEST_F(ComponentTest, Context)
{
    auto comp = ComponentPtr::Adopt(Component_Create(context->getObject(), nullptr, StringPtr("child"), nullptr));
    ASSERT_EQ(context->getObject(), comp.getContext());
}

TEST_F(ComponentTest, NullPropertyClass)
{
    auto comp = ComponentPtr::Adopt(Component_Create(context->getObject(), nullptr, StringPtr("child"), nullptr));
    ASSERT_EQ(comp.getClassName(), "");
}

TEST_F(ComponentTest, PropertyClass)
{
    auto context = Context(nullptr, Logger(), TypeManager(), nullptr, nullptr);
    auto rangeItemClass = PropertyObjectClassBuilder("TestClass").build();
    context.getTypeManager().addType(rangeItemClass);

    auto comp = ComponentPtr::Adopt(Component_Create(context, nullptr, StringPtr("child"), StringPtr("TestClass")));
    ASSERT_EQ(comp.getClassName(), "TestClass");
}

TEST_F(ComponentTest, Tags)
{
    auto comp = ComponentPtr::Adopt(Component_Create(context->getObject(), nullptr, StringPtr("comp"), nullptr));
    auto tags = comp.getTags();
    ASSERT_TRUE(tags.assigned());
}

TEST_F(ComponentTest, Name)
{
    auto comp = ComponentPtr::Adopt(Component_Create(context->getObject(), nullptr, StringPtr("comp"), nullptr));
    auto name = comp.getName();
    ASSERT_EQ(name, "comp");
}

TEST_F(ComponentTest, StandardProperties)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = Component(NullContext(), nullptr, "Temp");

    component.setName(name);
    component.setDescription(desc);

    ASSERT_EQ(component.getName(), name);
    ASSERT_EQ(component.getDescription(), desc);
}

TEST_F(ComponentTest, SerializeAndUpdate)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = Component(NullContext(), nullptr, "Temp");

    component.setName(name);
    component.setDescription(desc);

    const auto serializer = JsonSerializer(True);
    component.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto newComponent = Component(NullContext(), nullptr, "Temp");
    const auto deserializer = JsonDeserializer();
    const auto updatable = newComponent.asPtr<IUpdatable>();

    deserializer.update(updatable, str1);

    ASSERT_EQ(newComponent.getName(), name);
    ASSERT_EQ(newComponent.getDescription(), desc);

    const auto serializer2 = JsonSerializer(True);
    newComponent.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ComponentTest, SerializeAndDeserialize)
{
    const auto ctx = NullContext();
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = Component(ctx, nullptr, "Temp");

    const auto typeManager = component.getContext().getTypeManager();
    const auto statusType = EnumerationType("StatusType", List<IString>("Off", "On"));
    typeManager.addType(statusType);
    const auto statusValue = Enumeration("StatusType", "On", typeManager);

    component.getStatusContainer().asPtr<IComponentStatusContainerPrivate>().addStatus("Status", statusValue);
    component.setName(name);
    component.setDescription(desc);
    component.getTags().asPtr<ITagsPrivate>().add("Tag");

    component.addProperty(IntPropertyBuilder("Prop", 2).build());
    component.setPropertyValue("Prop", 3);

    const auto serializer = JsonSerializer(True);
    component.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const auto deserializeContext = ComponentDeserializeContext(ctx, nullptr, nullptr, "Temp");

    const ComponentPtr newComponent = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(newComponent.getName(), name);
    ASSERT_EQ(newComponent.getDescription(), desc);
    ASSERT_EQ(newComponent.getTags(), component.getTags());
    ASSERT_EQ(newComponent.getPropertyValue("Prop"), component.getPropertyValue("Prop"));
    ASSERT_EQ(newComponent.getStatusContainer().getStatuses(), component.getStatusContainer().getStatuses());

    const auto serializer2 = JsonSerializer(True);
    newComponent.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ComponentTest, SerializeName)
{
    const auto component = Component(NullContext(), nullptr, "Temp");
    const auto serializer = JsonSerializer(True);
    component.serialize(serializer);
    const std::string str = serializer.getOutput();
    ASSERT_TRUE(str.find("name") != std::string::npos);
}

TEST_F(ComponentTest, LockedProperties)
{
    const auto component = Component(NullContext(), nullptr, "Temp");

    ASSERT_EQ(component.getLockedAttributes().getCount(), 1u);

    ASSERT_NO_THROW(component.setName("Name"));
    ASSERT_NO_THROW(component.setDescription("Desc"));
    ASSERT_NO_THROW(component.setVisible(false));
    ASSERT_NO_THROW(component.setActive(false));

    ASSERT_EQ(component.getName(), "Name");
    ASSERT_EQ(component.getDescription(), "Desc");
    ASSERT_EQ(component.getVisible(), true);
    ASSERT_EQ(component.getActive(), false);

    component.asPtr<IComponentPrivate>().lockAllAttributes();

    ASSERT_NO_THROW(component.setName("ignored"));
    ASSERT_NO_THROW(component.setDescription("ignored"));
    ASSERT_NO_THROW(component.setVisible(false));
    ASSERT_NO_THROW(component.setActive(true));

    ASSERT_EQ(component.getName(), "Name");
    ASSERT_EQ(component.getDescription(), "Desc");
    ASSERT_EQ(component.getVisible(), true);
    ASSERT_EQ(component.getActive(), false);
    
    component.asPtr<IComponentPrivate>().unlockAllAttributes();
    
    ASSERT_NO_THROW(component.setName("not_ignored"));
    ASSERT_NO_THROW(component.setDescription("not_ignored"));
    ASSERT_NO_THROW(component.setVisible(false));
    ASSERT_NO_THROW(component.setActive(true));

    ASSERT_EQ(component.getName(), "not_ignored");
    ASSERT_EQ(component.getDescription(), "not_ignored");
    ASSERT_EQ(component.getVisible(), false);
    ASSERT_EQ(component.getActive(), true);
}

TEST_F(ComponentTest, StatusContainer)
{
    const auto component = Component(NullContext(), nullptr, "Temp");

    const auto componentStatusContainer = component.getStatusContainer();

    ASSERT_TRUE(componentStatusContainer.assigned());
}

TEST_F(ComponentTest, NonPropertyObjectObjectTypeProperty)
{
    const auto obj = PropertyObject();
    const auto component = Component(NullContext(), nullptr, "Temp");
    auto prop = ObjectProperty("Object1", component);

    ASSERT_THROW(obj.addProperty(prop), InvalidTypeException);
}

TEST_F(ComponentTest, Remove)
{
    auto component = Component(NullContext(), nullptr, "Temp");

    ASSERT_NO_THROW(component.remove());
    ASSERT_TRUE(component.isRemoved());

    ASSERT_THROW(component.getLockedAttributes().getCount(), ComponentRemovedException);
    ASSERT_THROW(component.asPtr<IComponentPrivate>().unlockAllAttributes(), ComponentRemovedException);
    ASSERT_THROW(component.asPtr<IComponentPrivate>().lockAllAttributes(), ComponentRemovedException);

    ASSERT_THROW(component.setName("ignored"), ComponentRemovedException);
    ASSERT_THROW(component.setDescription("ignored"), ComponentRemovedException);
    ASSERT_THROW(component.setVisible(false), ComponentRemovedException);
    ASSERT_THROW(component.setActive(true), ComponentRemovedException);
}

class MyTestComponent : public ComponentImpl<IComponent>
{
public:
    MyTestComponent(const ContextPtr& context,
        const ComponentPtr& parent)
        : ComponentImpl<IComponent>(context, parent, "foo")
    {
    }

    void initComponentErrorStateStatusPublic()
    {
        this->initComponentErrorStateStatus();
    }

    void setComponentErrorStateStatusPublic(const ComponentErrorState& status)
    {
        this->setComponentErrorStateStatus(status);
    }
};

TEST_F(ComponentTest, SetComponentErrorStateStatusWithoutInit)
{
    auto component = createWithImplementation<IComponent, MyTestComponent>(NullContext(), nullptr);
    auto implPtr = dynamic_cast<MyTestComponent*>(component.getObject());

    ASSERT_THROW_MSG(implPtr->setComponentErrorStateStatusPublic(ComponentErrorState::Error),
                     NotFoundException,
                     "ComponentStatus has not been added to statusContainer. initComponentErrorStateStatus needs to be called before "
                     "setComponentErrorStateStatus.")
}

TEST_F(ComponentTest, InitThenSetComponentErrorStateStatus)
{
    auto component = createWithImplementation<IComponent, MyTestComponent>(NullContext(), nullptr);
    auto implPtr = dynamic_cast<MyTestComponent*>(component.getObject());
    auto container = component.getStatusContainer();

    implPtr->initComponentErrorStateStatusPublic();

    ASSERT_EQ(
        container.getStatus("ComponentStatus"),
        EnumerationWithIntValue("ComponentStatusType", static_cast<Int>(ComponentErrorState::Ok), component.getContext().getTypeManager()));

    ASSERT_EQ(container.getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Ok", component.getContext().getTypeManager()));

    implPtr->setComponentErrorStateStatusPublic(ComponentErrorState::Error);

    ASSERT_EQ(container.getStatus("ComponentStatus"),
              EnumerationWithIntValue(
                  "ComponentStatusType", static_cast<Int>(ComponentErrorState::Error), component.getContext().getTypeManager()));

    ASSERT_EQ(container.getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Error", component.getContext().getTypeManager()));

    implPtr->setComponentErrorStateStatusPublic(ComponentErrorState::Warning);

    ASSERT_EQ(container.getStatus("ComponentStatus"),
              EnumerationWithIntValue(
                  "ComponentStatusType", static_cast<Int>(ComponentErrorState::Warning), component.getContext().getTypeManager()));

    ASSERT_EQ(container.getStatus("ComponentStatus"), Enumeration("ComponentStatusType", "Warning", component.getContext().getTypeManager()));
}
