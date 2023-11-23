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

    ASSERT_EQ(comp.getGlobalId(), "parent/child");
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
    const auto component = Component(nullptr, nullptr, "temp");

    component.setName(name);
    component.setDescription(desc);

    ASSERT_EQ(component.getName(), name);
    ASSERT_EQ(component.getDescription(), desc);
}

TEST_F(ComponentTest, SerializeAndUpdate)
{
    const auto name = "foo";
    const auto desc = "bar";
    const auto component = Component(nullptr, nullptr, "temp");

    component.setName(name);
    component.setDescription(desc);

    const auto serializer = JsonSerializer(True);
    component.serialize(serializer);
    const auto str1 = serializer.getOutput();

    const auto newComponent = Component(nullptr, nullptr, "temp");
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