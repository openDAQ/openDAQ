#include <gtest/gtest.h>
#include <opcuatms/core_types_utils.h>
#include <open62541/types_generated_handling.h>

using CoreTypesUtilsTest = testing::Test;

using namespace daq;
using namespace daq::opcua;
using namespace daq::opcua;

TEST_F(CoreTypesUtilsTest, ConvertToDaqCoreString)
{
    UA_String testString = UA_String_fromChars("test");
    StringPtr rtString = ConvertToDaqCoreString(testString);
    ASSERT_EQ(rtString.toStdString(), "test");

    UA_String_clear(&testString);
}

TEST_F(CoreTypesUtilsTest, ConvertToDaqCoreStringNull)
{
    UA_String testString = UA_STRING_NULL;
    StringPtr rtString = ConvertToDaqCoreString(testString);
    ASSERT_FALSE(rtString.assigned());

    UA_String_clear(&testString);
}

TEST_F(CoreTypesUtilsTest, ConvertToDaqCoreStringEmpty)
{
    UA_String testString = UA_String_fromChars("");
    StringPtr rtString = ConvertToDaqCoreString(testString);
    ASSERT_EQ(rtString.toStdString(), "");

    UA_String_clear(&testString);
}

TEST_F(CoreTypesUtilsTest, ConvertToOpcUaString)
{
    StringPtr rtString = "test";

    OpcUaObject<UA_String> str = ConvertToOpcUaString(rtString);
    rtString.release();

    ASSERT_TRUE(*str == "test");
}

TEST_F(CoreTypesUtilsTest, ConvertToOpcUaStringNull)
{
    StringPtr rtString;

    OpcUaObject<UA_String> str = ConvertToOpcUaString(rtString);
    rtString.release();

    ASSERT_TRUE(*str == UA_STRING_NULL);
}

TEST_F(CoreTypesUtilsTest, SampleTypeConverter)
{
    ASSERT_EQ(SampleTypeFromTmsEnum(SampleTypeToTmsEnum(SampleType::Int32)), SampleType::Int32);
    ASSERT_EQ(SampleTypeFromTmsEnum(SampleTypeToTmsEnum(SampleType::Int64)), SampleType::Int64);
    ASSERT_EQ(SampleTypeFromTmsEnum(SampleTypeToTmsEnum(SampleType::UInt64)), SampleType::UInt64);
}
