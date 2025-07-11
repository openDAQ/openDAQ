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

TEST_F(ArgumentInfoTest, SerializeDeserializeList)
{
    auto argInfo1 = ArgumentInfo("Int", ctInt);
    auto argInfo2 = ArgumentInfo("Float", ctFloat);
    const auto argInfoList = ContainerArgumentInfo("Name", CoreType::ctList, List<IArgumentInfo>(argInfo1, argInfo2));

    const auto serializer = JsonSerializer();
    argInfoList.serialize(serializer);

    const auto jsonStr = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const ArgumentInfoPtr argInfo = deserializer.deserialize(jsonStr);
    ASSERT_EQ(argInfo.getName(), argInfoList.getName());
    ASSERT_EQ(argInfo.getType(), argInfoList.getType());
    ASSERT_EQ(argInfo.getContainerArgumentInfo(), argInfoList.getContainerArgumentInfo());
}

TEST_F(ArgumentInfoTest, SerializeDeserializeDict)
{
    auto argInfo1 = ArgumentInfo("Int", ctInt);
    auto argInfo2 = ArgumentInfo("Float", ctFloat);
    const auto argInfoDict = ContainerArgumentInfo("Name", CoreType::ctDict, List<IArgumentInfo>(argInfo1, argInfo2));

    const auto serializer = JsonSerializer();
    argInfoDict.serialize(serializer);

    const auto jsonStr = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const ArgumentInfoPtr argInfo = deserializer.deserialize(jsonStr);
    ASSERT_EQ(argInfo.getName(), argInfoDict.getName());
    ASSERT_EQ(argInfo.getType(), argInfoDict.getType());
    ASSERT_EQ(argInfo.getContainerArgumentInfo(), argInfoDict.getContainerArgumentInfo());
}

TEST_F(ArgumentInfoTest, SerializeDeserializeListNested)
{
    auto argInfo1 = ArgumentInfo("Int", ctInt);

    auto argInfoInner1 = ArgumentInfo("Enum", ctEnumeration);
    auto argInfoInner2 = ArgumentInfo("Float", ctFloat);
    auto argInfo2 = ContainerArgumentInfo("Float", ctList, List<IArgumentInfo>(argInfoInner1, argInfoInner2));

    const auto argInfoList = ContainerArgumentInfo("Name", CoreType::ctList, List<IArgumentInfo>(argInfo1, argInfo2));

    const auto serializer = JsonSerializer();
    argInfoList.serialize(serializer);

    const auto jsonStr = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const ArgumentInfoPtr argInfo = deserializer.deserialize(jsonStr);
    ASSERT_EQ(argInfo.getName(), argInfoList.getName());
    ASSERT_EQ(argInfo.getType(), argInfoList.getType());
    ASSERT_EQ(argInfo.getContainerArgumentInfo(), argInfoList.getContainerArgumentInfo());
}
