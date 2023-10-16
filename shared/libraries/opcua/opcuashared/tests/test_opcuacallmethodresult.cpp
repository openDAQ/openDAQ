#include "gtest/gtest.h"
#include "opcuashared/opcuacallmethodresult.h"
#include <open62541/types_generated_handling.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaCallMethodResultTest = testing::Test;

TEST_F(OpcUaCallMethodResultTest, Create)
{
    UA_CallMethodResult callMethodResult;
    UA_CallMethodResult_init(&callMethodResult);

    callMethodResult.statusCode = UA_STATUSCODE_BADAGGREGATELISTMISMATCH;

    callMethodResult.outputArgumentsSize = 2;
    callMethodResult.outputArguments = (UA_Variant*) UA_Array_new(callMethodResult.outputArgumentsSize, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_Int64 intVal = 0;
    UA_Variant_setScalarCopy(&callMethodResult.outputArguments[0], &intVal, &UA_TYPES[UA_TYPES_INT64]);

    UA_String strVal = UA_STRING_ALLOC("Test");
    UA_Variant_setScalarCopy(&callMethodResult.outputArguments[1], &strVal, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&strVal);

    OpcUaCallMethodResult value(callMethodResult);

    ASSERT_EQ(value.getOutputArgumentsSize(), 2u);
    ASSERT_EQ(value.getStatusCode(), UA_STATUSCODE_BADAGGREGATELISTMISMATCH);

    OpcUaVariant val1 = value.getOutputArgument(0);
    ASSERT_TRUE(val1.isInteger());
    ASSERT_EQ(val1.toInteger(), 0);

    OpcUaVariant val2 = value.getOutputArgument(1);
    ASSERT_TRUE(val2.isString());
    ASSERT_STREQ(val2.toString().c_str(), "Test");

    ASSERT_THROW(value.getOutputArgument(2), std::out_of_range);

    UA_CallMethodResult_clear(&callMethodResult);
}

END_NAMESPACE_OPENDAQ_OPCUA
