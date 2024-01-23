#include <gtest/gtest.h>
#include <opendaq/gmock/context.h>
#include <opendaq/gmock/component.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/context_factory.h>

using ComponentDeserializeContextTest = testing::Test;

TEST_F(ComponentDeserializeContextTest, Create)
{
    MockContext::Strict context;
    daq::MockComponent::Strict parent;

    const auto deserializeContext = daq::ComponentDeserializeContext(context.ptr, parent.ptr, "id");

    ASSERT_EQ(deserializeContext.getContext(), context.ptr);
    ASSERT_EQ(deserializeContext.getParent(), parent.ptr);
    ASSERT_EQ(deserializeContext.getLocalId(), "id");
}

TEST_F(ComponentDeserializeContextTest, Clone)
{
    MockContext::Strict context;
    daq::MockComponent::Strict parent;

    const auto deserializeContext = daq::ComponentDeserializeContext(context.ptr, parent.ptr, "id");

    daq::MockComponent::Strict newParent;

    const auto newDeserializeContext = deserializeContext.clone(newParent, "newId");

    ASSERT_EQ(newDeserializeContext.getContext(), context.ptr);
    ASSERT_EQ(newDeserializeContext.getParent(), newParent.ptr);
    ASSERT_EQ(newDeserializeContext.getLocalId(), "newId");
}

TEST_F(ComponentDeserializeContextTest, QueryInterfaceTypeManager)
{
    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, "id");

    const auto typeManager = deserializeContext.asPtr<daq::ITypeManager>();
    ASSERT_TRUE(typeManager.assigned());
}

TEST_F(ComponentDeserializeContextTest, BorrowInterfaceTypeManager)
{
    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, "id");

    const auto typeManager = deserializeContext.asPtr<daq::ITypeManager>(true);
    ASSERT_TRUE(typeManager.assigned());
}
