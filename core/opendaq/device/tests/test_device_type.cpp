#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <testutils/testutils.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>

using DeviceTypeTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Dict equality ignores order, so the order is checked through the key list
TEST_F(DeviceTypeTest, AvailableTransportsDefault)
{
    const DeviceTypePtr factoryType = DeviceType("id", "name", "description", "prefix");
    ASSERT_EQ(factoryType.getAvailableTransports().getCount(), 0u);

    const DeviceTypePtr builtType = DeviceTypeBuilder().setId("id").setConnectionStringPrefix("prefix").build();
    ASSERT_EQ(builtType.getAvailableTransports().getCount(), 0u);
}

TEST_F(DeviceTypeTest, BuilderSetsAvailableTransports)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setConnectionStringPrefix("prefix")
                                   .setAvailableTransports(Dict<IString, IInteger>({{"tls", 7421}, {"plain", 7420}}))
                                   .build();

    const auto transports = type.getAvailableTransports();
    ASSERT_EQ(transports.getKeyList(), List<IString>("tls", "plain"));
    ASSERT_EQ(transports.get("tls"), 7421);
    ASSERT_EQ(transports.get("plain"), 7420);
}

TEST_F(DeviceTypeTest, BuilderSetAvailableTransportsReplaces)
{
    const auto builder = DeviceTypeBuilder()
                             .setAvailableTransports(Dict<IString, IInteger>({{"plain", 7420}, {"tls", 7421}}))
                             .setAvailableTransports(Dict<IString, IInteger>({{"https", 443}}));

    ASSERT_EQ(builder.getAvailableTransports().getKeyList(), List<IString>("https"));
}

TEST_F(DeviceTypeTest, BuilderSetAvailableTransportsInvalid)
{
    const auto builder = DeviceTypeBuilder();

    ASSERT_THROW(builder.setAvailableTransports(nullptr), ArgumentNullException);
    ASSERT_THROW(builder.setAvailableTransports(Dict<IString, IString>({{"plain", "7420"}})), InvalidTypeException);
    ASSERT_THROW(builder.setAvailableTransports(Dict<IInteger, IInteger>({{1, 7420}})), InvalidTypeException);
    ASSERT_EQ(builder.getAvailableTransports().getCount(), 0u);
}

TEST_F(DeviceTypeTest, AvailableTransportsAreFrozenCopy)
{
    auto transports = Dict<IString, IInteger>({{"plain", 7420}});
    const DeviceTypePtr type = DeviceTypeBuilder().setId("id").setAvailableTransports(transports).build();

    transports.set("tls", 7421);

    ASSERT_EQ(type.getAvailableTransports().getKeyList(), List<IString>("plain"));
    ASSERT_THROW(type.getAvailableTransports().set("wss", 7443), FrozenException);
}

TEST_F(DeviceTypeTest, SerializeDeserializeAvailableTransports)
{
    const DeviceTypePtr type = DeviceTypeBuilder()
                                   .setId("id")
                                   .setName("name")
                                   .setDescription("description")
                                   .setConnectionStringPrefix("prefix")
                                   .setAvailableTransports(Dict<IString, IInteger>({{"wss", 7443}, {"tls", 7421}, {"plain", 7420}}))
                                   .build();

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const DeviceTypePtr newType = deserializer.deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getId(), "id");
    ASSERT_EQ(newType.getConnectionStringPrefix(), "prefix");

    const auto transports = newType.getAvailableTransports();
    ASSERT_EQ(transports.getKeyList(), List<IString>("wss", "tls", "plain"));
    ASSERT_EQ(transports.get("wss"), 7443);

    serializer.reset();
    newType.serialize(serializer);
    ASSERT_EQ(serializedType, serializer.getOutput());
}

// Types without transports serialize exactly as an older SDK did, and the older format deserializes to an empty dictionary
TEST_F(DeviceTypeTest, SerializeWithoutAvailableTransports)
{
    const DeviceTypePtr type = DeviceType("id", "name", "description", "prefix");

    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const auto serializedType = serializer.getOutput();
    ASSERT_EQ(std::string(serializedType).find("availableTransports"), std::string::npos);

    const auto deserializer = JsonDeserializer();
    const DeviceTypePtr newType = deserializer.deserialize(serializedType, nullptr, nullptr);

    ASSERT_EQ(newType.getAvailableTransports().getCount(), 0u);
}

END_NAMESPACE_OPENDAQ
