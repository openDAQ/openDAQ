#include <coretypes/coretypes.h>
#include <gtest/gtest.h>
#include <opendaq/connection_status_container_impl.h>
#include <opendaq/connection_status_container_private_ptr.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_deserialize_context_factory.h>
#include <opendaq/mock/mock_streaming_factory.h>

using namespace daq;
using namespace testing;

using ConnectionStatusContainerTest = testing::Test;

static ComponentStatusContainerPtr ConnectionStatusContainer()
{
    return createWithImplementation<IComponentStatusContainer, ConnectionStatusContainerImpl>();
}

TEST_F(ConnectionStatusContainerTest, Create)
{
    const auto connectionStatusContainer = ConnectionStatusContainer();

    ASSERT_TRUE(connectionStatusContainer.assigned());
}

TEST_F(ConnectionStatusContainerTest, EmptyStatuses)
{
    const auto connectionStatusContainer = ConnectionStatusContainer();

    const auto dict = connectionStatusContainer.getStatuses();
    ASSERT_EQ(dict.getCount(), 0u);

    ASSERT_THROW(connectionStatusContainer.getStatus("ConfigurationStatus"), NotFoundException);
    ASSERT_THROW(connectionStatusContainer.getStatus("StreamingStatus_1"), NotFoundException);
    ASSERT_THROW(connectionStatusContainer.getStatusMessage("ConfigurationStatus"), NotFoundException);
    ASSERT_THROW(connectionStatusContainer.getStatusMessage("StreamingStatus_1"), NotFoundException);
}

TEST_F(ConnectionStatusContainerTest, AddConfigStatus)
{
    const auto context = NullContext();
    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", context.getTypeManager());

    const auto connectionStatusContainer = ConnectionStatusContainer();
    const auto connectionString = String("ConnectionString");
    auto connectionStatusContainerPrivate = connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>();

    ASSERT_THROW(connectionStatusContainerPrivate.addConfigurationConnectionStatus(nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.addConfigurationConnectionStatus(connectionString, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.addConfigurationConnectionStatus(nullptr, statusInitValue), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.addConfigurationConnectionStatus("", statusInitValue), InvalidParameterException);

    ASSERT_NO_THROW(connectionStatusContainerPrivate.addConfigurationConnectionStatus(connectionString, statusInitValue));
    ASSERT_THROW(connectionStatusContainerPrivate.addConfigurationConnectionStatus(connectionString, statusInitValue), AlreadyExistsException);

    const auto dict = connectionStatusContainer.getStatuses();
    ASSERT_EQ(dict.getCount(), 1u);
    ASSERT_TRUE(dict.hasKey("ConfigurationStatus"));

    EnumerationPtr statusValue;
    ASSERT_NO_THROW(statusValue = connectionStatusContainer.getStatus("ConfigurationStatus"));
    ASSERT_EQ(statusValue, statusInitValue);

    StringPtr statusMessage;
    ASSERT_NO_THROW(statusMessage = connectionStatusContainer.getStatusMessage("ConfigurationStatus"));
    ASSERT_EQ(statusMessage, "");
}

TEST_F(ConnectionStatusContainerTest, AddStreamingStatus)
{
    const auto context = NullContext();
    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", context.getTypeManager());

    const auto connectionStatusContainer = ConnectionStatusContainer();
    const auto connectionString = String("ConnectionString");
    const StreamingPtr mockStreamingObject = MockStreaming("MockStreaming", context);
    auto connectionStatusContainerPrivate = connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>();

    ASSERT_THROW(connectionStatusContainerPrivate.addStreamingConnectionStatus(nullptr, nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.addStreamingConnectionStatus(connectionString, nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.addStreamingConnectionStatus(nullptr, statusInitValue, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.addStreamingConnectionStatus(nullptr, nullptr, mockStreamingObject), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.addStreamingConnectionStatus("", statusInitValue, mockStreamingObject), InvalidParameterException);

    ASSERT_NO_THROW(connectionStatusContainerPrivate.addStreamingConnectionStatus(connectionString, statusInitValue, mockStreamingObject));
    ASSERT_THROW(connectionStatusContainerPrivate.addStreamingConnectionStatus(connectionString, statusInitValue, mockStreamingObject), AlreadyExistsException);

    const auto dict = connectionStatusContainer.getStatuses();
    ASSERT_EQ(dict.getCount(), 1u);
    ASSERT_TRUE(dict.hasKey("StreamingStatus_1"));

    EnumerationPtr statusValue;
    ASSERT_NO_THROW(statusValue = connectionStatusContainer.getStatus("StreamingStatus_1"));
    ASSERT_EQ(statusValue, statusInitValue);

    StringPtr statusMessage;
    ASSERT_NO_THROW(statusMessage = connectionStatusContainer.getStatusMessage("StreamingStatus_1"));
    ASSERT_EQ(statusMessage, "");
}

TEST_F(ConnectionStatusContainerTest, AddConfigAndStreamingStatuses)
{
    const auto context = NullContext();
    const auto typeManager = context.getTypeManager();
    const auto connected = Enumeration("ConnectionStatusType", "Connected", typeManager);
    const auto reconnecting = Enumeration("ConnectionStatusType", "Reconnecting", typeManager);
    const auto unrecoverable = Enumeration("ConnectionStatusType", "Unrecoverable", typeManager);

    const auto connectionStatusContainer = ConnectionStatusContainer();
    const auto configConnectionString = String("ConfigConnectionString");
    const auto streamingConnectionString1 = String("StreamingConnectionString1");
    const auto streamingConnectionString2 = String("StreamingConnectionString2");
    const StreamingPtr mockStreamingObject = MockStreaming("MockStreaming", context);
    auto statusContainer = connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>();

    ASSERT_NO_THROW(statusContainer.addStreamingConnectionStatus(streamingConnectionString1, connected, mockStreamingObject));
    ASSERT_THROW(statusContainer.addStreamingConnectionStatus(streamingConnectionString1, reconnecting, mockStreamingObject), AlreadyExistsException);

    ASSERT_THROW(statusContainer.addConfigurationConnectionStatus(streamingConnectionString1, unrecoverable), AlreadyExistsException);
    ASSERT_NO_THROW(statusContainer.addConfigurationConnectionStatus(configConnectionString, unrecoverable));

    ASSERT_THROW(statusContainer.addStreamingConnectionStatus(configConnectionString, reconnecting, mockStreamingObject), AlreadyExistsException);
    ASSERT_THROW(statusContainer.addStreamingConnectionStatus(streamingConnectionString1, reconnecting, mockStreamingObject), AlreadyExistsException);
    ASSERT_NO_THROW(statusContainer.addStreamingConnectionStatus(streamingConnectionString2, reconnecting, mockStreamingObject));

    const auto dict = connectionStatusContainer.getStatuses();
    ASSERT_EQ(dict.getCount(), 3u);
    ASSERT_TRUE(dict.hasKey("StreamingStatus_1"));
    ASSERT_TRUE(dict.hasKey("StreamingStatus_2"));
    ASSERT_TRUE(dict.hasKey("ConfigurationStatus"));

    EnumerationPtr statusValue;
    ASSERT_NO_THROW(statusValue = connectionStatusContainer.getStatus("StreamingStatus_1"));
    ASSERT_EQ(statusValue, "Connected");
    ASSERT_NO_THROW(statusValue = connectionStatusContainer.getStatus("StreamingStatus_2"));
    ASSERT_EQ(statusValue, "Reconnecting");
    ASSERT_NO_THROW(statusValue = connectionStatusContainer.getStatus("ConfigurationStatus"));
    ASSERT_EQ(statusValue, "Unrecoverable");
}

TEST_F(ConnectionStatusContainerTest, RemoveAddStreamingStatus)
{
    const auto context = NullContext();
    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", context.getTypeManager());

    const auto connectionStatusContainer = ConnectionStatusContainer();
    const auto connectionString1 = String("ConnectionString1");
    const auto connectionString2 = String("ConnectionString2");
    const StreamingPtr mockStreamingObject = MockStreaming("MockStreaming", context);
    auto connectionStatusContainerPrivate = connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>();

    ASSERT_THROW(connectionStatusContainerPrivate.removeStreamingConnectionStatus(connectionString1), NotFoundException);
    connectionStatusContainerPrivate.addStreamingConnectionStatus(connectionString1, statusInitValue, mockStreamingObject);
    connectionStatusContainerPrivate.addStreamingConnectionStatus(connectionString2, statusInitValue, mockStreamingObject);

    ASSERT_NO_THROW(connectionStatusContainerPrivate.removeStreamingConnectionStatus(connectionString1));
    ASSERT_THROW(connectionStatusContainerPrivate.removeStreamingConnectionStatus(connectionString1), NotFoundException);

    const auto dict = connectionStatusContainer.getStatuses();
    ASSERT_EQ(dict.getCount(), 1u);

    EnumerationPtr statusValue;
    ASSERT_THROW(statusValue = connectionStatusContainer.getStatus("StreamingStatus_1"), NotFoundException);
    StringPtr statusMessage;
    ASSERT_THROW(statusMessage = connectionStatusContainer.getStatusMessage("StreamingStatus_1"), NotFoundException);

    connectionStatusContainerPrivate.addStreamingConnectionStatus(connectionString1, statusInitValue, mockStreamingObject);
    ASSERT_NO_THROW(statusValue = connectionStatusContainer.getStatus("StreamingStatus_3"));
}

TEST_F(ConnectionStatusContainerTest, SetStatus)
{
    const auto context = NullContext();
    const auto typeManager = context.getTypeManager();
    const auto otherType = EnumerationType("OtherType", List<IString>("zero", "one"));
    typeManager.addType(otherType);

    const auto otherTypeValue = Enumeration("OtherType", "zero", typeManager);
    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", typeManager);
    const auto statusNewValue = Enumeration("ConnectionStatusType", "Reconnecting", typeManager);

    const auto connectionStatusContainer = ConnectionStatusContainer();
    const auto connectionString = String("ConnectionString");
    auto connectionStatusContainerPrivate = connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>();

    connectionStatusContainerPrivate.addConfigurationConnectionStatus(connectionString, statusInitValue);

    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus(nullptr, nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus(connectionString, nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus(nullptr, statusNewValue, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus("", statusNewValue, nullptr), InvalidParameterException);
    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus(connectionString, otherTypeValue, nullptr), InvalidTypeException);

    ASSERT_EQ(connectionStatusContainerPrivate->updateConnectionStatus(connectionString, statusInitValue, nullptr), OPENDAQ_IGNORED);
    ASSERT_NO_THROW(connectionStatusContainerPrivate.updateConnectionStatus(connectionString, statusNewValue, nullptr));

    EnumerationPtr statusValue;
    ASSERT_NO_THROW(statusValue = connectionStatusContainer.getStatus("ConfigurationStatus"));
    ASSERT_EQ(statusValue, statusNewValue);

    StringPtr statusMessage;
    ASSERT_NO_THROW(statusMessage = connectionStatusContainer.getStatusMessage("ConfigurationStatus"));
    ASSERT_EQ(statusMessage, "");
}

TEST_F(ConnectionStatusContainerTest, SetStatusWithMessage)
{
    const auto context = NullContext();
    const auto typeManager = context.getTypeManager();
    const auto otherType = EnumerationType("OtherType", List<IString>("zero", "one"));
    typeManager.addType(otherType);

    const auto otherTypeValue = Enumeration("OtherType", "zero", typeManager);
    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", typeManager);
    const auto statusNewValue = Enumeration("ConnectionStatusType", "Reconnecting", typeManager);

    const auto connectionStatusContainer = ConnectionStatusContainer();
    const auto connectionString = String("ConnectionString");
    auto connectionStatusContainerPrivate = connectionStatusContainer.asPtr<IConnectionStatusContainerPrivate>();

    connectionStatusContainerPrivate.addConfigurationConnectionStatus(connectionString, statusInitValue);

    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus(nullptr, nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus(connectionString, nullptr, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus(nullptr, statusNewValue, nullptr), ArgumentNullException);
    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus("", statusNewValue, nullptr), InvalidParameterException);
    ASSERT_THROW(connectionStatusContainerPrivate.updateConnectionStatus(connectionString, otherTypeValue, nullptr), InvalidTypeException);

    ASSERT_EQ(connectionStatusContainerPrivate->updateConnectionStatus(connectionString, statusInitValue, nullptr), OPENDAQ_IGNORED);
    ASSERT_EQ(connectionStatusContainerPrivate->updateConnectionStatusWithMessage(connectionString, statusInitValue, nullptr, String("")), OPENDAQ_IGNORED);
    ASSERT_NO_THROW(connectionStatusContainerPrivate.updateConnectionStatus(connectionString, statusNewValue, nullptr));
    ASSERT_NO_THROW(connectionStatusContainerPrivate.updateConnectionStatusWithMessage(connectionString, statusNewValue, nullptr, "New message"));

    EnumerationPtr statusValue;
    ASSERT_NO_THROW(statusValue = connectionStatusContainer.getStatus("ConfigurationStatus"));
    ASSERT_EQ(statusValue, statusNewValue);

    StringPtr statusMessage;
    ASSERT_NO_THROW(statusMessage = connectionStatusContainer.getStatusMessage("ConfigurationStatus"));
    ASSERT_EQ(statusMessage, "New message");
}

TEST_F(ConnectionStatusContainerTest, SerializeDeserialize)
{
    const auto context = NullContext();
    const auto typeManager = context.getTypeManager();

    const auto statusInitValue = Enumeration("ConnectionStatusType", "Connected", typeManager);
    const auto statusContainer = ConnectionStatusContainer();
    const auto configConnectionString = String("ConfigConnectionString");
    const auto streamingConnectionString = String("StreamingConnectionString");
    auto connectionStatusContainerPrivate = statusContainer.asPtr<IConnectionStatusContainerPrivate>();
    const auto mockStreaming = MockStreaming("MockStreaming", context);

    connectionStatusContainerPrivate.addConfigurationConnectionStatus(configConnectionString, statusInitValue);
    connectionStatusContainerPrivate.addStreamingConnectionStatus(streamingConnectionString, statusInitValue, mockStreaming);

    connectionStatusContainerPrivate.updateConnectionStatusWithMessage(configConnectionString, statusInitValue, nullptr, "Config connection status message");
    connectionStatusContainerPrivate.updateConnectionStatusWithMessage(streamingConnectionString, statusInitValue, mockStreaming, "Streaming connection status message");

    auto serializer = JsonSerializer(False);
    statusContainer.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(context, nullptr, nullptr, nullptr);
    ComponentStatusContainerPtr deserializedStatusContainer = deserializer.deserialize(serialized, deserializeContext);

    // TODO streaming statuses are not serialized/deserialized
    // ASSERT_EQ(deserializedStatusContainer.getStatuses(), statusContainer.getStatuses());
    // test config status only
    ASSERT_EQ(deserializedStatusContainer.getStatuses().getCount(), 1u);
    ASSERT_EQ(deserializedStatusContainer.getStatus("ConfigurationStatus"),
              statusContainer.getStatus("ConfigurationStatus"));
    ASSERT_EQ(deserializedStatusContainer.getStatusMessage("ConfigurationStatus"),
              "Config connection status message");
    ASSERT_FALSE(deserializedStatusContainer.getStatuses().hasKey("StreamingStatus_1"));
}
