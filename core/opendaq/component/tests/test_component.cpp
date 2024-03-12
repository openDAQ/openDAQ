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
    auto context = Context(nullptr, Logger(), TypeManager(), nullptr);
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
    const auto component = Component(NullContext(), nullptr, "temp");

    component.setName(name);
    component.setDescription(desc);

    ASSERT_EQ(component.getName(), name);
    ASSERT_EQ(component.getDescription(), desc);
}

TEST_F(ComponentTest, SerializeAndUpdate)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = Component(NullContext(), nullptr, "temp");

    component.setName(name);
    component.setDescription(desc);

    const auto serializer = JsonSerializer(True);
    component.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto newComponent = Component(NullContext(), nullptr, "temp");
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
    const auto component = Component(ctx, nullptr, "temp");

    const auto typeManager = component.getContext().getTypeManager();
    const auto statusType = EnumerationType("StatusType", List<IString>("Off", "On"));
    typeManager.addType(statusType);
    const auto statusValue = Enumeration("StatusType", "On", typeManager);

    component.getStatusContainer().asPtr<IComponentStatusContainerPrivate>().addStatus("status", statusValue);
    component.setName(name);
    component.setDescription(desc);
    component.getTags().asPtr<ITagsPrivate>().add("tag");

    component.addProperty(IntPropertyBuilder("prop", 2).build());
    component.setPropertyValue("prop", 3);

    const auto serializer = JsonSerializer(True);
    component.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const auto deserializeContext = ComponentDeserializeContext(ctx, nullptr, nullptr, "temp");

    const ComponentPtr newComponent = deserializer.deserialize(str1, deserializeContext, nullptr);

    ASSERT_EQ(newComponent.getName(), name);
    ASSERT_EQ(newComponent.getDescription(), desc);
    ASSERT_EQ(newComponent.getTags(), component.getTags());
    ASSERT_EQ(newComponent.getPropertyValue("prop"), component.getPropertyValue("prop"));
    ASSERT_EQ(newComponent.getStatusContainer().getStatuses(), component.getStatusContainer().getStatuses());

    const auto serializer2 = JsonSerializer(True);
    newComponent.serialize(serializer2);
    const auto str2 = serializer2.getOutput();

    ASSERT_EQ(str1, str2);
}

TEST_F(ComponentTest, LockedProperties)
{
    const auto component = Component(NullContext(), nullptr, "temp");

    ASSERT_EQ(component.getLockedAttributes().getCount(), 1u);

    ASSERT_NO_THROW(component.setName("name"));
    ASSERT_NO_THROW(component.setDescription("desc"));
    ASSERT_NO_THROW(component.setVisible(false));
    ASSERT_NO_THROW(component.setActive(false));

    ASSERT_EQ(component.getName(), "name");
    ASSERT_EQ(component.getDescription(), "desc");
    ASSERT_EQ(component.getVisible(), true);
    ASSERT_EQ(component.getActive(), false);

    component.asPtr<IComponentPrivate>().lockAllAttributes();

    ASSERT_NO_THROW(component.setName("ignored"));
    ASSERT_NO_THROW(component.setDescription("ignored"));
    ASSERT_NO_THROW(component.setVisible(false));
    ASSERT_NO_THROW(component.setActive(true));

    ASSERT_EQ(component.getName(), "name");
    ASSERT_EQ(component.getDescription(), "desc");
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
    const auto component = Component(NullContext(), nullptr, "temp");

    const auto componentStatusContainer = component.getStatusContainer();

    ASSERT_TRUE(componentStatusContainer.assigned());
}

TEST_F(ComponentTest, NonPropertyObjectObjectTypeProperty)
{
    const auto obj = PropertyObject();
    const auto component = Component(NullContext(), nullptr, "temp");
    auto prop = ObjectProperty("Object1", component);

    ASSERT_THROW(obj.addProperty(prop), InvalidTypeException);
}
