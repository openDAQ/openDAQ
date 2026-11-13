#include <opendaq/server_type_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>

using ServerTypeTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// These tests exercise the still-functional but deprecated ServerType factory
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4996)
#endif

TEST_F(ServerTypeTest, ServerTypeFactory)
{
    ServerTypePtr serverType;
    ASSERT_NO_THROW(serverType = ServerType("test_server_type", "", ""));
    ASSERT_EQ(serverType.getId(), "test_server_type");

    const auto config = serverType.createDefaultConfig();
    ASSERT_TRUE(config.assigned());
    ASSERT_EQ(config.getAllProperties().getCount(), 0u);
}

TEST_F(ServerTypeTest, ServerTypeFactoryWithDefaultConfig)
{
    ServerTypePtr serverType;

    auto defaultConfig = PropertyObject();
    auto info = PropertyBuilder("Timeout").setDefaultValue(2).setValueType(ctFloat).build();
    defaultConfig.addProperty(info);

    ASSERT_NO_THROW(serverType = ServerType("test_uid", "", "", defaultConfig));

    auto defaultConfigClone = serverType.createDefaultConfig();
    ASSERT_TRUE(defaultConfigClone.assigned());
    ASSERT_EQ(defaultConfigClone.getPropertyValue("Timeout"), 2);
    defaultConfigClone.setPropertyValue("Timeout", 44);
    ASSERT_EQ(serverType.createDefaultConfig().getPropertyValue("Timeout"), 2);
}

TEST_F(ServerTypeTest, DefaultConfigNullValue)
{
    ServerTypePtr serverType = ServerType("test_uid", "", "", nullptr);
    ASSERT_TRUE(serverType.createDefaultConfig().assigned());
    ASSERT_EQ(serverType.createDefaultConfig().getAllProperties().getCount(), 0u);
}

TEST_F(ServerTypeTest, ServerTypeStructType)
{
    const auto structType = daq::ServerTypeStructType();
    const daq::StructPtr structPtr = daq::ServerType("Id", "Name", "Desc");
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(ServerTypeTest, ServerTypeStructFields)
{
    const daq::StructPtr structPtr = daq::ServerType("Id", "Name", "Desc");
    ASSERT_EQ(structPtr.get("Id"), "Id");
    ASSERT_EQ(structPtr.get("Name"), "Name");
    ASSERT_EQ(structPtr.get("Description"), "Desc");
}

TEST_F(ServerTypeTest, ServerTypeStructNames)
{
    const auto structType = daq::ServerTypeStructType();
    const daq::StructPtr structPtr = daq::ServerType("Id", "Name", "Desc");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(ServerTypeTest, FactoryKindUnspecified)
{
    ASSERT_EQ(ServerType("id", "", "").getServerKind(), ServerKind::Unspecified);
}

TEST_F(ServerTypeTest, BuilderKind)
{
    const auto builder = ServerTypeBuilder().setId("id");
    ASSERT_EQ(builder.getServerKind(), ServerKind::Unspecified);
    ASSERT_EQ(builder.build().asPtr<IServerType>().getServerKind(), ServerKind::Unspecified);

    builder.setServerKind(ServerKind::Streaming);
    ASSERT_EQ(builder.getServerKind(), ServerKind::Streaming);
    ASSERT_EQ(builder.build().asPtr<IServerType>().getServerKind(), ServerKind::Streaming);
}

TEST_F(ServerTypeTest, BuilderOfOtherSortStoresKind)
{
    const auto builder = DeviceTypeBuilder().setId("dev").setServerKind(ServerKind::Mirror);
    ASSERT_EQ(builder.getServerKind(), ServerKind::Mirror);
    ASSERT_NO_THROW(builder.build());
}

TEST_F(ServerTypeTest, KindValues)
{
    ASSERT_EQ(static_cast<int>(ServerKind::Unspecified), 0);
    ASSERT_EQ(static_cast<int>(ServerKind::Adapter), 1);
    ASSERT_EQ(static_cast<int>(ServerKind::Mirror), 2);
    ASSERT_EQ(static_cast<int>(ServerKind::Streaming), 3);
}

TEST_F(ServerTypeTest, SerializeKind)
{
    const auto type = ServerTypeBuilder().setId("id").setName("name").setServerKind(ServerKind::Adapter).build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const ServerTypePtr deserialized = JsonDeserializer().deserialize(serializer.getOutput());

    ASSERT_EQ(deserialized.getId(), "id");
    ASSERT_EQ(deserialized.getName(), "name");
    ASSERT_EQ(deserialized.getServerKind(), ServerKind::Adapter);
}

TEST_F(ServerTypeTest, DeserializeWithoutKind)
{
    const ServerTypePtr deserialized = JsonDeserializer().deserialize(R"({"__type":"ServerType","id":"id"})");

    ASSERT_EQ(deserialized.getId(), "id");
    ASSERT_EQ(deserialized.getServerKind(), ServerKind::Unspecified);
}

TEST_F(ServerTypeTest, DeserializeUnknownKind)
{
    const ServerTypePtr deserialized = JsonDeserializer().deserialize(R"({"__type":"ServerType","id":"id","serverKind":99})");

    ASSERT_EQ(deserialized.getServerKind(), ServerKind::Unspecified);
}

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

END_NAMESPACE_OPENDAQ
