#include <coreobjects/argument_info_factory.h>
#include <coreobjects/callable_info_factory.h>
#include <gtest/gtest.h>

using CallableInfoTest = testing::Test;

using namespace daq;

/*
 * Function
 */

TEST_F(CallableInfoTest, CreateFunctionReturnOnly)
{
    ASSERT_NO_THROW(FunctionInfo(CoreType::ctBool));
}

TEST_F(CallableInfoTest, CreateFunctionEmpty)
{
    ASSERT_NO_THROW(FunctionInfo(CoreType::ctBool, List<IArgumentInfo>()));
}

TEST_F(CallableInfoTest, CreateFunctionOneArg)
{
    ASSERT_NO_THROW(FunctionInfo(CoreType::ctBool, List<IArgumentInfo>(ArgumentInfo("first", CoreType::ctFloat))));
}

/*
 * Procedure
 */

TEST_F(CallableInfoTest, CreateProcedureNoArgs)
{
    ASSERT_NO_THROW(ProcedureInfo());
}

TEST_F(CallableInfoTest, CreateProcedureEmpty)
{
    ASSERT_NO_THROW(ProcedureInfo(List<IArgumentInfo>()));
}

TEST_F(CallableInfoTest, CreateProcedureOneArg)
{
    ASSERT_NO_THROW(ProcedureInfo(List<IArgumentInfo>(ArgumentInfo("first", CoreType::ctFloat))));
}

/*
 * Getters
 */

TEST_F(CallableInfoTest, ProcedureReturnType)
{
    auto procInfo = ProcedureInfo();
    ASSERT_EQ(procInfo.getReturnType(), CoreType::ctUndefined);
}

TEST_F(CallableInfoTest, FunctionReturnType)
{
    auto funcInfo = FunctionInfo(CoreType::ctFloat);
    ASSERT_EQ(funcInfo.getReturnType(), CoreType::ctFloat);
}

TEST_F(CallableInfoTest, Arguments)
{
    auto argList = List<IArgumentInfo>(ArgumentInfo("first", CoreType::ctFloat));
    auto procInfo = ProcedureInfo(argList);

    ASSERT_EQ(procInfo.getArguments(), argList);
}

TEST_F(CallableInfoTest, ProcInspectable)
{
    auto obj = ProcedureInfo();

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], ICallableInfo::Id);
}

TEST_F(CallableInfoTest, ProcImplementationName)
{
    auto obj = ProcedureInfo();

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::CallableInfoImpl");
}

TEST_F(CallableInfoTest, FuncInspectable)
{
    auto obj = FunctionInfo(CoreType::ctFloat);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], ICallableInfo::Id);
}

TEST_F(CallableInfoTest, FuncImplementationName)
{
    auto obj = FunctionInfo(CoreType::ctFloat);

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::CallableInfoImpl");
}

TEST_F(CallableInfoTest, StructType)
{
    const auto structType = CallableInfoStructType();
    const StructPtr structPtr = FunctionInfo(ctString);
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(CallableInfoTest, StructFields)
{
    const StructPtr structPtr = FunctionInfo(ctString);

    ASSERT_EQ(structPtr.get("returnType"), static_cast<Int>(ctString));
    ASSERT_EQ(structPtr.get("arguments"), nullptr);
}

TEST_F(CallableInfoTest, StructNames)
{
    const auto structType = CallableInfoStructType();
    const StructPtr structPtr = FunctionInfo(ctString);
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(CallableInfoTest, SerializeDeserialize)
{
    const auto functionInfo1 = FunctionInfo(ctInt, List<IArgumentInfo>(ArgumentInfo("name", CoreType::ctString)));

    const auto serializer = JsonSerializer();
    functionInfo1.serialize(serializer);

    const auto jsonStr = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const CallableInfoPtr functionInfo2 = deserializer.deserialize(jsonStr);
    ASSERT_EQ(functionInfo1, functionInfo2);
}
