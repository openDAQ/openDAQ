#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <config_protocol/component_holder_factory.h>
#include <opendaq/gmock/component.h>
#include <opendaq/component_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/context_factory.h>

using namespace daq;
using namespace config_protocol;

using ComponentHolderTest = testing::Test;

TEST_F(ComponentHolderTest, NullComponent)
{
    MockComponent::Strict component;

    ASSERT_THROW(ComponentHolder(nullptr, nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(ComponentHolder("id", nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(ComponentHolder(nullptr, nullptr, component.ptr), ArgumentNullException);
}

TEST_F(ComponentHolderTest, ComponentWithId)
{
    MockComponent::Strict component;
    const auto componentHolder = ComponentHolder("id", "parent_id", component.ptr);
    ASSERT_EQ(componentHolder.getLocalId(), "id");
    ASSERT_EQ(componentHolder.getComponent(), component.ptr);
    ASSERT_EQ(componentHolder.getParentGlobalId(), "parent_id");
}

TEST_F(ComponentHolderTest, Component)
{
    MockComponent::Strict component;
    EXPECT_CALL(component.mock(), getLocalId(testing::_)).WillOnce(Get<StringPtr>(String("id")));
    EXPECT_CALL(component.mock(), getParent(testing::_)).WillOnce(Get<ComponentPtr>(nullptr));
    const auto componentHolder = ComponentHolder(component.ptr);
    ASSERT_EQ(componentHolder.getLocalId(), "id");
    ASSERT_EQ(componentHolder.getComponent(), component.ptr);
}

TEST_F(ComponentHolderTest, SerializeDeserialize)
{
    const auto ctx = NullContext();
    const auto sourceComponent = Component(ctx, nullptr, "id");
    const auto componentHolder = ComponentHolder("id", "parent", sourceComponent);

    const auto serializer = JsonSerializer();
    componentHolder.serialize(serializer);
    const auto serializedComponentHolder = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const auto parentComponent = Component(ctx, nullptr, "parent");


    const auto deserializeContext = ComponentDeserializeContext(ctx, parentComponent, parentComponent, nullptr);

    const ComponentHolderPtr newComponentHolder = deserializer.deserialize(serializedComponentHolder, deserializeContext, nullptr);

    ASSERT_EQ(newComponentHolder.getLocalId(), "id");
    const auto newComponent = newComponentHolder.getComponent();

    ASSERT_EQ(newComponent.getLocalId(), "id");
    ASSERT_EQ(newComponent.getContext(), ctx);
    ASSERT_EQ(newComponent.getParent(), parentComponent);
}
