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
    daq::IntfID intfID = daq::IBoolean::Id;

    const auto deserializeContext = daq::ComponentDeserializeContext(context.ptr, parent.ptr, parent.ptr, "Id", &intfID);

    ASSERT_EQ(deserializeContext.getContext(), context.ptr);
    ASSERT_EQ(deserializeContext.getParent(), parent.ptr);
    ASSERT_EQ(deserializeContext.getLocalId(), "Id");
    ASSERT_EQ(deserializeContext.getIntfID(), daq::IBoolean::Id);
}

TEST_F(ComponentDeserializeContextTest, Clone)
{
    MockContext::Strict context;
    daq::MockComponent::Strict parent;
    daq::IntfID intfID = daq::IBoolean::Id;

    const auto deserializeContext = daq::ComponentDeserializeContext(context.ptr, parent.ptr, parent.ptr, "Id", &intfID);

    daq::MockComponent::Strict newParent;

    daq::IntfID newIntfID = daq::IInteger::Id;
    const auto newDeserializeContext = deserializeContext.clone(newParent, "newId", &newIntfID);

    ASSERT_EQ(newDeserializeContext.getContext(), context.ptr);
    ASSERT_EQ(newDeserializeContext.getParent(), newParent.ptr);
    ASSERT_EQ(newDeserializeContext.getLocalId(), "newId");
    ASSERT_EQ(newDeserializeContext.getIntfID(), daq::IInteger::Id);
}

TEST_F(ComponentDeserializeContextTest, QueryInterfaceTypeManager)
{
    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "Id");

    const auto typeManager = deserializeContext.asPtr<daq::ITypeManager>();
    ASSERT_TRUE(typeManager.assigned());
}

TEST_F(ComponentDeserializeContextTest, BorrowInterfaceTypeManager)
{
    const auto deserializeContext = daq::ComponentDeserializeContext(daq::NullContext(), nullptr, nullptr, "Id");

    const auto typeManager = deserializeContext.asPtr<daq::ITypeManager>(true);
    ASSERT_TRUE(typeManager.assigned());
}
