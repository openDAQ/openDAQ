#include <gtest/gtest.h>
#include <opcuashared/opcuaobject.h>
#include <open62541/types_generated_handling.h>
#include <opcuashared/opcuanodeid.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaObjectTest = testing::Test;

TEST_F(OpcUaObjectTest, CreateSimpleType)
{
    OpcUaObject<UA_Int16> simpleVal;

    UA_Int16 value;
    UA_Int16_init(&value);

    ASSERT_EQ(simpleVal.getValue(), value);
}

TEST_F(OpcUaObjectTest, CreateComplexType)
{
    OpcUaObject<UA_Variant> complexVal;

    ASSERT_TRUE(UA_Variant_isEmpty(complexVal.get()));
}

TEST_F(OpcUaObjectTest, SimpleTypeCopyConstructor)
{
    UA_Int32 ua_val = 5;

    OpcUaObject<UA_Int32> variant(ua_val);

    const UA_Int32& val = variant.getValue();

    ASSERT_EQ(val, 5);
}

TEST_F(OpcUaObjectTest, ComplexTypeCopyConstructor)
{
    UA_Variant ua_variant;
    UA_Variant_init(&ua_variant);

    double value = 5;
    UA_Variant_setScalarCopy(&ua_variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]);

    OpcUaObject<UA_Variant> object(ua_variant);

    const UA_Variant& var = object.getValue();

    UA_Variant_hasScalarType(&var, &UA_TYPES[UA_TYPES_DOUBLE]);
    ASSERT_EQ(*static_cast<double*>(var.data), value);

    UA_Variant_clear(&ua_variant);
}

TEST_F(OpcUaObjectTest, SimpleTypeMoveConstructor)
{
    UA_Int32 ua_val = 5;

    OpcUaObject<UA_Int32> variant(std::move(ua_val));

    ASSERT_EQ(ua_val, 0);

    const UA_Int32& val = variant.getValue();

    ASSERT_EQ(val, 5);
}

TEST_F(OpcUaObjectTest, ComplexTypeMoveConstructor)
{
    UA_Variant ua_variant;
    double value = 5;
    UA_Variant_setScalarCopy(&ua_variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]);

    OpcUaObject<UA_Variant> variant(std::move(ua_variant));
    ASSERT_TRUE(UA_Variant_isEmpty(&ua_variant));

    const UA_Variant& var = variant.getValue();

    UA_Variant_hasScalarType(&var, &UA_TYPES[UA_TYPES_DOUBLE]);
    ASSERT_EQ(*static_cast<double*>(var.data), value);
}

TEST_F(OpcUaObjectTest, SimpleTypeMoveAssignment)
{
    OpcUaObject<int32_t> object(5);
    object = OpcUaObject<int32_t>(6);

    ASSERT_EQ(object.getValue(), 6);
}

TEST_F(OpcUaObjectTest, ComplexTypeMoveAssignment)
{
    OpcUaObject<UA_String> object(UA_STRING_ALLOC("Test"));
    object = OpcUaObject<UA_String>(UA_STRING_ALLOC("New val"));

    UA_String result = UA_STRING_ALLOC("New val");

    ASSERT_TRUE(*object == result);

    UA_String_clear(&result);
}

TEST_F(OpcUaObjectTest, SimpleTypeSetValueCopy)
{
    UA_Int16 ua_int = 14;

    OpcUaObject<UA_Int16> object;
    object.setValue(ua_int);

    ASSERT_EQ(object.getValue(), 14);
}

TEST_F(OpcUaObjectTest, ComplexTypeSetValueCopy)
{
    UA_String ua_str = UA_STRING_ALLOC("Test");

    OpcUaObject<UA_String> object;
    object.setValue(ua_str);

    UA_String_clear(&ua_str);

    ASSERT_TRUE(*object == "Test");
}

TEST_F(OpcUaObjectTest, SimpleTypeSetValueMove)
{
    OpcUaObject<UA_Int16> object;
    object.setValue(14);

    ASSERT_EQ(object.getValue(), 14);
}

TEST_F(OpcUaObjectTest, ComplexTypeSetValueMove)
{
    UA_String str = UA_STRING_ALLOC("Test");

    OpcUaObject<UA_String> object;
    object.setValue(std::move(str));

    ASSERT_EQ(str.data, nullptr);
    ASSERT_EQ(str.length, 0u);

    ASSERT_TRUE(*object == "Test");
}

TEST_F(OpcUaObjectTest, SimpleTypeCopyConstructOperator)
{
    OpcUaObject<int32_t> variant(5);
    OpcUaObject<int32_t> variant1 = variant;

    ASSERT_EQ(variant.getValue(), 5);
    ASSERT_EQ(variant1.getValue(), 5);
}

TEST_F(OpcUaObjectTest, ComplexTypeCopyConstructOperator)
{
    OpcUaObject variant(UA_STRING_ALLOC("Test"));
    OpcUaObject variant1 = variant;

    ASSERT_TRUE(variant.getValue() == "Test");
    ASSERT_TRUE(variant1.getValue() == "Test");
}

TEST_F(OpcUaObjectTest, SimpleTypeAssignmentOperator)
{
    OpcUaObject<int32_t> variant(5);
    OpcUaObject<int32_t> variant1;
    variant1 = variant;

    ASSERT_EQ(variant.getValue(), 5);
    ASSERT_EQ(variant1.getValue(), 5);
}

TEST_F(OpcUaObjectTest, ComplexTypeAssignmentOperator)
{
    OpcUaObject<UA_String> variant(UA_STRING_ALLOC("Test"));
    OpcUaObject<UA_String> variant1;

    variant1 = variant;

    ASSERT_TRUE(variant.getValue() == "Test");
    ASSERT_TRUE(variant1.getValue() == "Test");
}

TEST_F(OpcUaObjectTest, ArrowOperator)
{
    OpcUaObject<UA_String> object(UA_STRING_ALLOC("Test"));

    const OpcUaObject<UA_String>& objectConst = object;
    ASSERT_EQ(objectConst->length, 4u);
    ASSERT_EQ(object->length, 4u);
}

TEST_F(OpcUaObjectTest, SimpleTypeClear)
{
    OpcUaObject<int32_t> object(4);

    ASSERT_EQ(object.getValue(), 4);

    object.clear();

    ASSERT_EQ(object.getValue(), 0);
}

TEST_F(OpcUaObjectTest, ComplexTypeClear)
{
    OpcUaObject<UA_String> object(UA_STRING_ALLOC("Test"));

    ASSERT_TRUE(object.getValue() == "Test");

    object.clear();

    ASSERT_TRUE(object.getValue() == UA_STRING_NULL);
}

TEST_F(OpcUaObjectTest, SimpleTypeGetDetachedValue)
{
    OpcUaObject<int32_t> object(4);

    ASSERT_EQ(object.getValue(), 4);

    object.getDetachedValue();

    ASSERT_EQ(object.getValue(), 0);
}

TEST_F(OpcUaObjectTest, ComplexTypeGetDetachedValue)
{
    UA_String text = UA_STRING_ALLOC("Test");

    OpcUaObject<UA_String> object(text);

    UA_String value = object.getDetachedValue();

    ASSERT_TRUE(value == text);
    ASSERT_TRUE(object.getValue() == UA_STRING_NULL);

    UA_String_clear(&value);
    UA_String_clear(&text);
}

TEST_F(OpcUaObjectTest, SelfAssign)
{
    auto targetNodeId = OpcUaNodeId(1, 1000);
    auto nodeId = OpcUaNodeId(1, 1000);
    OpcUaNodeId* pNodeId = &nodeId;
    nodeId = nodeId;
    ASSERT_EQ(pNodeId, &nodeId);
    ASSERT_EQ(targetNodeId, nodeId);
}

END_NAMESPACE_OPENDAQ_OPCUA
