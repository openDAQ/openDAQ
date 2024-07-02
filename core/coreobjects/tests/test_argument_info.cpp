#include <gtest/gtest.h>
#include <coreobjects/argument_info_factory.h>

using ArgumentInfoTest = testing::Test;

using namespace daq;

TEST_F(ArgumentInfoTest, Create)
{
    ASSERT_NO_THROW(ArgumentInfo("Name", CoreType::ctBool));
}

TEST_F(ArgumentInfoTest, Name)
{
    auto arg = ArgumentInfo("Name", CoreType::ctBool);
    ASSERT_EQ(arg.getName(), "Name");
}

TEST_F(ArgumentInfoTest, Type)
{
    auto arg = ArgumentInfo("Name", CoreType::ctBool);
    ASSERT_EQ(arg.getType(), CoreType::ctBool);
}

TEST_F(ArgumentInfoTest, Inspectable)
{
    auto obj = ArgumentInfo("Name", CoreType::ctBool);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IArgumentInfo::Id);
}

TEST_F(ArgumentInfoTest, ImplementationName)
{
    auto obj = ArgumentInfo("Name", CoreType::ctBool);

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::ArgumentInfoImpl");
}

TEST_F(ArgumentInfoTest, StructType)
{
    const auto structType = ArgumentInfoStructType();
    const StructPtr structPtr = ArgumentInfo("test", ctString);
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(ArgumentInfoTest, StructFields)
{
    const StructPtr structPtr = ArgumentInfo("test", ctString);
    ASSERT_EQ(structPtr.get("Name"), "test");
    ASSERT_EQ(structPtr.get("Type"), static_cast<Int>(ctString));
}

TEST_F(ArgumentInfoTest, StructNames)
{
    const auto structType = ArgumentInfoStructType();
    const StructPtr structPtr = ArgumentInfo("test", ctString);
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(ArgumentInfoTest, SerializeDeserialize)
{
    const auto argInfo1 = ArgumentInfo("Name", CoreType::ctBool);

    const auto serializer = JsonSerializer();
    argInfo1.serialize(serializer);

    const auto jsonStr = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const ArgumentInfoPtr argInfo = deserializer.deserialize(jsonStr);
    ASSERT_EQ(argInfo.getName(), argInfo1.getName());
    ASSERT_EQ(argInfo.getType(), argInfo1.getType());
}
