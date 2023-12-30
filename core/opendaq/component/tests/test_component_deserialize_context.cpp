#include <gtest/gtest.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/component.h>
#include <opendaq/component_deserialize_context_factory.h>

using ComponentDeserializeContextTest = testing::Test;

TEST_F(ComponentDeserializeContextTest, Create)
{
    MockContext::Strict context;
    daq::MockComponent::Strict parent;
    daq::TypeManagerPtr typeManager = daq::TypeManager();

    const auto deserializeContext = daq::ComponentDeserializeContext(context.ptr, parent.ptr, "id", typeManager);

    ASSERT_EQ(deserializeContext.getContext(), context.ptr);
    ASSERT_EQ(deserializeContext.getParent(), parent.ptr);
    ASSERT_EQ(deserializeContext.getLocalId(), "id");
    ASSERT_EQ(deserializeContext.getTypeManager(), typeManager);
}

TEST_F(ComponentDeserializeContextTest, Clone)
{
    MockContext::Strict context;
    daq::MockComponent::Strict parent;
    daq::TypeManagerPtr typeManager = daq::TypeManager();

    const auto deserializeContext = daq::ComponentDeserializeContext(context.ptr, parent.ptr, "id", typeManager);

    daq::MockComponent::Strict newParent;

    const auto newDeserializeContext = deserializeContext.clone(newParent, "newId");

    ASSERT_EQ(newDeserializeContext.getContext(), context.ptr);
    ASSERT_EQ(newDeserializeContext.getParent(), newParent.ptr);
    ASSERT_EQ(newDeserializeContext.getLocalId(), "newId");
    ASSERT_EQ(newDeserializeContext.getTypeManager(), typeManager);
}
