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
    daq::ProcedurePtr triggerCoreEvent = [](const daq::CoreEventArgsPtr&) {};

    const auto deserializeContext = daq::ComponentDeserializeContext(context.ptr, parent.ptr, parent.ptr, "Id", &intfID, triggerCoreEvent);

    ASSERT_EQ(deserializeContext.getContext(), context.ptr);
    ASSERT_EQ(deserializeContext.getParent(), parent.ptr);
    ASSERT_EQ(deserializeContext.getLocalId(), "Id");
    ASSERT_EQ(deserializeContext.getIntfID(), daq::IBoolean::Id);
    ASSERT_EQ(deserializeContext.getTriggerCoreEvent(), triggerCoreEvent);
}

TEST_F(ComponentDeserializeContextTest, Clone)
{
    MockContext::Strict context;
    daq::MockComponent::Strict parent;
    daq::IntfID intfID = daq::IBoolean::Id;

    const auto deserializeContext = daq::ComponentDeserializeContext(context.ptr, parent.ptr, parent.ptr, "Id", &intfID);

    daq::MockComponent::Strict newParent;

    daq::IntfID newIntfID = daq::IInteger::Id;
    daq::ProcedurePtr newTriggerCoreEvent = [](const daq::CoreEventArgsPtr&) {};
    const auto newDeserializeContext = deserializeContext.clone(newParent, "newId", &newIntfID, newTriggerCoreEvent);

    ASSERT_EQ(newDeserializeContext.getContext(), context.ptr);
    ASSERT_EQ(newDeserializeContext.getParent(), newParent.ptr);
    ASSERT_EQ(newDeserializeContext.getLocalId(), "newId");
    ASSERT_EQ(newDeserializeContext.getIntfID(), daq::IInteger::Id);
    ASSERT_EQ(newDeserializeContext.getTriggerCoreEvent(), newTriggerCoreEvent);
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
