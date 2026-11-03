#include <testutils/testutils.h>
#include <opendaq/streaming_type_factory.h>
#include <opendaq/device_type_ptr.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/component_type_private.h>
#include <opendaq/module_info_factory.h>
#include <coretypes/version_info_factory.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

using StreamingTypeTest = testing::Test;

static StreamingTypePtr roundTrip(const StreamingTypePtr& type, std::string* jsonOut = nullptr)
{
    const auto serializer = JsonSerializer();
    type.serialize(serializer);
    const std::string json = serializer.getOutput();
    if (jsonOut)
        *jsonOut = json;
    return JsonDeserializer().deserialize(json, nullptr, nullptr);
}

TEST_F(StreamingTypeTest, SerializeFull)
{
    auto defaultConfig = PropertyObject();
    defaultConfig.addProperty(IntProperty("IntProp", 5));
    defaultConfig.addProperty(StringProperty("StrProp", "foo"));

    const StreamingTypePtr type = StreamingTypeBuilder()
                                      .setId("id")
                                      .setName("name")
                                      .setDescription("desc")
                                      .setConnectionStringPrefix("prefix")
                                      .setDefaultConfig(defaultConfig)
                                      .build();
    type.asPtr<IComponentTypePrivate>()->setModuleInfo(ModuleInfo(VersionInfo(1, 2, 3), "module_name", "module_id"));

    std::string json;
    const StreamingTypePtr deserialized = roundTrip(type, &json);
    ASSERT_NE(json.find("\"__type\":\"StreamingType\""), std::string::npos) << json;

    ASSERT_EQ(deserialized, type);
    ASSERT_EQ(deserialized.getId(), "id");
    ASSERT_EQ(deserialized.getName(), "name");
    ASSERT_EQ(deserialized.getDescription(), "desc");
    ASSERT_EQ(deserialized.getConnectionStringPrefix(), "prefix");
    ASSERT_FALSE(deserialized.getLinkedDeviceType().assigned());

    const auto config = deserialized.createDefaultConfig();
    ASSERT_EQ(config.getPropertyValue("IntProp"), 5);
    ASSERT_EQ(config.getPropertyValue("StrProp"), "foo");

    const auto moduleInfo = deserialized.getModuleInfo();
    ASSERT_TRUE(moduleInfo.assigned());
    ASSERT_EQ(moduleInfo.getId(), "module_id");
    ASSERT_EQ(moduleInfo.getName(), "module_name");
    ASSERT_EQ(moduleInfo.getVersionInfo().getMajor(), 1u);
    ASSERT_EQ(moduleInfo.getVersionInfo().getMinor(), 2u);
    ASSERT_EQ(moduleInfo.getVersionInfo().getPatch(), 3u);
}

TEST_F(StreamingTypeTest, SerializeLinked)
{
    auto deviceConfig = PropertyObject();
    deviceConfig.addProperty(IntProperty("Port", 7420));
    const auto deviceType = DeviceTypeBuilder().setId("linkedDevType").setConnectionStringPrefix("dev").setDefaultConfig(deviceConfig).build();

    const StreamingTypePtr type =
        StreamingTypeBuilder().setId("id").setConnectionStringPrefix("prefix").setLinkedDeviceType(deviceType).build();

    const StreamingTypePtr deserialized = roundTrip(type);

    ASSERT_EQ(deserialized, type);
    // Equality does not compare the link, so check it explicitly
    ASSERT_EQ(deserialized.getLinkedDeviceType(), deviceType);
    ASSERT_EQ(deserialized.createDefaultConfig().getPropertyValue("Port"), 7420);
}

TEST_F(StreamingTypeTest, DeserializeMinimal)
{
    const std::string json = R"({"__type":"StreamingType","id":"id"})";
    const StreamingTypePtr deserialized = JsonDeserializer().deserialize(json, nullptr, nullptr);
    ASSERT_EQ(deserialized.getId(), "id");
    ASSERT_FALSE(deserialized.getLinkedDeviceType().assigned());
}

END_NAMESPACE_OPENDAQ
