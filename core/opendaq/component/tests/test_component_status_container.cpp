#include <coretypes/coretypes.h>
#include <gtest/gtest.h>
#include <opendaq/component_status_container_factory.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/instance_factory.h>

using namespace daq;
using namespace testing;

using ComponentStatusContainerTest = testing::Test;

TEST_F(ComponentStatusContainerTest, Create)
{
    const auto componentStatusContainer = ComponentStatusContainer();

    ASSERT_TRUE(componentStatusContainer.assigned());
}

TEST_F(ComponentStatusContainerTest, EmptyStatuses)
{
    const auto componentStatusContainer = ComponentStatusContainer();

    const auto dict = componentStatusContainer.getStatuses();
    ASSERT_EQ(dict.getCount(), 0u);

    ASSERT_THROW(componentStatusContainer.getStatus("notExist"), NotFoundException);
}

TEST_F(ComponentStatusContainerTest, AddStatus)
{
    const auto typeManager = TypeManager();
    const auto statusType = EnumerationType("ComponentStatusContainerType", List<IString>("On", "Off"));
    typeManager.addType(statusType);
    const auto statusInitValue = Enumeration("ComponentStatusContainerType", "On", typeManager);

    const auto componentStatusContainer = ComponentStatusContainer();
    const auto statusName = String("testStatus");
    auto componentStatusContainerPrivate = componentStatusContainer.asPtr<IComponentStatusContainerPrivate>();

    ASSERT_THROW(componentStatusContainerPrivate.addStatus(nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(componentStatusContainerPrivate.addStatus(statusName, nullptr), ArgumentNullException);
    ASSERT_THROW(componentStatusContainerPrivate.addStatus(nullptr, statusInitValue), ArgumentNullException);
    ASSERT_THROW(componentStatusContainerPrivate.addStatus("", statusInitValue), InvalidParameterException);

    ASSERT_NO_THROW(componentStatusContainerPrivate.addStatus(statusName, statusInitValue));
    ASSERT_THROW(componentStatusContainerPrivate.addStatus(statusName, statusInitValue), AlreadyExistsException);

    const auto dict = componentStatusContainer.getStatuses();
    ASSERT_EQ(dict.getCount(), 1u);
    ASSERT_TRUE(dict.hasKey(statusName));

    EnumerationPtr statusValue;
    ASSERT_NO_THROW(statusValue = componentStatusContainer.getStatus(statusName));
    ASSERT_EQ(statusValue, statusInitValue);
}

TEST_F(ComponentStatusContainerTest, SetStatus)
{
    const auto typeManager = TypeManager();
    const auto statusType = EnumerationType("ComponentStatusContainerType", List<IString>("On", "Off"));
    const auto otherType = EnumerationType("OtherType", List<IString>("zero", "one"));
    typeManager.addType(statusType);
    typeManager.addType(otherType);

    const auto statusInitValue = Enumeration("ComponentStatusContainerType", "On", typeManager);
    const auto statusNewValue = Enumeration("ComponentStatusContainerType", "Off", typeManager);
    const auto otherTypeValue = Enumeration("OtherType", "zero", typeManager);

    const auto componentStatusContainer = ComponentStatusContainer();
    const auto statusName = String("testStatus");
    auto componentStatusContainerPrivate = componentStatusContainer.asPtr<IComponentStatusContainerPrivate>();

    componentStatusContainerPrivate.addStatus(statusName, statusInitValue);

    ASSERT_THROW(componentStatusContainerPrivate.setStatus(nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(componentStatusContainerPrivate.setStatus(statusName, nullptr), ArgumentNullException);
    ASSERT_THROW(componentStatusContainerPrivate.setStatus(nullptr, statusNewValue), ArgumentNullException);
    ASSERT_THROW(componentStatusContainerPrivate.setStatus("", statusNewValue), InvalidParameterException);
    ASSERT_THROW(componentStatusContainerPrivate.setStatus(statusName, otherTypeValue), InvalidTypeException);

    ASSERT_EQ(componentStatusContainerPrivate->setStatus(statusName, statusInitValue), OPENDAQ_IGNORED);
    ASSERT_NO_THROW(componentStatusContainerPrivate.setStatus(statusName, statusNewValue));

    EnumerationPtr statusValue;
    ASSERT_NO_THROW(statusValue = componentStatusContainer.getStatus(statusName));
    ASSERT_EQ(statusValue, statusNewValue);
}

TEST_F(ComponentStatusContainerTest, SerializeDeserialize)
{
    const auto ctx = NullContext();
    const auto typeManager = ctx.getTypeManager();
    const auto statusType = EnumerationType("ComponentStatusContainerType", List<IString>("On", "Off"));
    typeManager.addType(statusType);

    const auto statusInitValue = Enumeration("ComponentStatusContainerType", "On", typeManager);

    const auto statusContainer = ComponentStatusContainer();
    const auto statusName = String("testStatus");
    auto componentStatusContainerPrivate = statusContainer.asPtr<IComponentStatusContainerPrivate>();
    componentStatusContainerPrivate.addStatus(statusName, statusInitValue);

    auto serializer = JsonSerializer(False);
    statusContainer.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(ctx, nullptr, nullptr, nullptr);
    ComponentStatusContainerPtr deserializedStatusContainer = deserializer.deserialize(serialized, deserializeContext);

    ASSERT_EQ(deserializedStatusContainer.getStatuses(), statusContainer.getStatuses());

    // Status message is and empty string
    ASSERT_EQ(deserializedStatusContainer.getStatusMessage(statusName), "");
}

TEST_F(ComponentStatusContainerTest, SerializeDeserializeWithMessage)
{
    const auto ctx = NullContext();
    const auto typeManager = ctx.getTypeManager();
    const auto statusType = EnumerationType("ComponentStatusContainerType", List<IString>("On", "Off"));
    typeManager.addType(statusType);

    const auto statusInitValue = Enumeration("ComponentStatusContainerType", "On", typeManager);

    const auto statusContainer = ComponentStatusContainer();
    const auto statusName = String("testStatus");
    const auto statusMessage = String("testMessage");
    auto componentStatusContainerPrivate = statusContainer.asPtr<IComponentStatusContainerPrivate>();
    componentStatusContainerPrivate.addStatusWithMessage(statusName, statusInitValue, statusMessage);

    auto serializer = JsonSerializer(False);
    statusContainer.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(ctx, nullptr, nullptr, nullptr);
    ComponentStatusContainerPtr deserializedStatusContainer = deserializer.deserialize(serialized, deserializeContext);

    ASSERT_EQ(deserializedStatusContainer.getStatuses(), statusContainer.getStatuses());
    ASSERT_EQ(deserializedStatusContainer.getStatusMessage(statusName), statusMessage);

    // Some additional tests for setStatusWithMessage and getStatusMessage
    const auto newStatusMessage = String("222testMessage222");
    componentStatusContainerPrivate.setStatusWithMessage(statusName, statusInitValue, newStatusMessage);
    ASSERT_EQ(statusContainer.getStatusMessage(statusName), newStatusMessage);
}
