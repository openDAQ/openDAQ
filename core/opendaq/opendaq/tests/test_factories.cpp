#include <opendaq/instance_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

using OpenDaqFactoriesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(OpenDaqFactoriesTest, ServerTypeBuilder)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const ServerTypePtr type = ServerTypeBuilder()
                               .setId("id")
                               .setName("name")
                               .setDescription("desc")
                               .setDefaultConfig(defConfig)
                               .build();

    ASSERT_EQ(type.getId(), "id");
    ASSERT_EQ(type.getName(), "name");
    ASSERT_EQ(type.getDescription(), "desc");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
}

TEST_F(OpenDaqFactoriesTest, DeviceTypeBuilder)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const DeviceTypePtr type = DeviceTypeBuilder()
                               .setId("id")
                               .setName("name")
                               .setDescription("desc")
                               .setConnectionStringPrefix("prefix")
                               .setDefaultConfig(defConfig)
                               .build();

    ASSERT_EQ(type.getId(), "id");
    ASSERT_EQ(type.getName(), "name");
    ASSERT_EQ(type.getDescription(), "desc");
    ASSERT_EQ(type.getConnectionStringPrefix(), "prefix");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
}

TEST_F(OpenDaqFactoriesTest, FBTypeBuilder)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const FunctionBlockTypePtr type = FunctionBlockTypeBuilder()
                               .setId("id")
                               .setName("name")
                               .setDescription("desc")
                               .setDefaultConfig(defConfig)
                               .build();

    ASSERT_EQ(type.getId(), "id");
    ASSERT_EQ(type.getName(), "name");
    ASSERT_EQ(type.getDescription(), "desc");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
}

TEST_F(OpenDaqFactoriesTest, StreamingTypeBuilder)
{
    auto defConfig = PropertyObject();
    defConfig.addProperty(StringProperty("foo", "bar"));

    const StreamingTypePtr type = StreamingTypeBuilder()
                               .setId("id")
                               .setName("name")
                               .setDescription("desc")
                               .setConnectionStringPrefix("prefix")
                               .setDefaultConfig(defConfig)
                               .build();

    ASSERT_EQ(type.getId(), "id");
    ASSERT_EQ(type.getName(), "name");
    ASSERT_EQ(type.getDescription(), "desc");
    ASSERT_EQ(type.getConnectionStringPrefix(), "prefix");
    ASSERT_EQ(type.createDefaultConfig().getPropertyValue("foo"), "bar");
}

END_NAMESPACE_OPENDAQ
