#include <opendaq/server_type_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

using ServerTypeTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

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
    ASSERT_FALSE(serverType.createDefaultConfig().assigned());
}

TEST_F(ServerTypeTest, ServerTypeStructType)
{
    const auto structType = daq::ServerTypeStructType();
    const daq::StructPtr structPtr = daq::ServerType("id", "name", "desc");
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(ServerTypeTest, ServerTypeStructFields)
{
    const daq::StructPtr structPtr = daq::ServerType("id", "name", "desc");
    ASSERT_EQ(structPtr.get("id"), "id");
    ASSERT_EQ(structPtr.get("name"), "name");
    ASSERT_EQ(structPtr.get("description"), "desc");
}

TEST_F(ServerTypeTest, ServerTypeStructNames)
{
    const auto structType = daq::ServerTypeStructType();
    const daq::StructPtr structPtr = daq::ServerType("id", "name", "desc");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

END_NAMESPACE_OPENDAQ
