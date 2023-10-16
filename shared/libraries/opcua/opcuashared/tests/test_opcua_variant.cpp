#include "gtest/gtest.h"
#include "opcuashared/opcuavariant.h"
#include <open62541/types_generated_handling.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaVariantTest = testing::Test;

TEST_F(OpcUaVariantTest, CreateNull)
{
    OpcUaVariant variant;
    ASSERT_FALSE(variant.isBool());
    ASSERT_FALSE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_FALSE(variant.isNodeId());
}

TEST_F(OpcUaVariantTest, CreateBool)
{
    OpcUaVariant variant(true);
    ASSERT_TRUE(variant.isBool());
    ASSERT_FALSE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_FALSE(variant.isNodeId());
    ASSERT_EQ(variant.toBool(), true);
}

TEST_F(OpcUaVariantTest, CreateDouble)
{
    OpcUaVariant variant(5.5);
    ASSERT_FALSE(variant.isBool());
    ASSERT_TRUE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_FALSE(variant.isNodeId());
    ASSERT_EQ(variant.toDouble(), 5.5);
}

TEST_F(OpcUaVariantTest, CreateInt)
{
    OpcUaVariant variant(int64_t(5));
    ASSERT_FALSE(variant.isBool());
    ASSERT_FALSE(variant.isDouble());
    ASSERT_TRUE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_FALSE(variant.isNodeId());
    ASSERT_EQ(variant.toInteger(), 5);
}

TEST_F(OpcUaVariantTest, CreateString)
{
    OpcUaVariant variant("Hi");
    ASSERT_FALSE(variant.isBool());
    ASSERT_FALSE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_TRUE(variant.isString());
    ASSERT_FALSE(variant.isNodeId());
    ASSERT_STREQ(variant.toString().c_str(), "Hi");
}

TEST_F(OpcUaVariantTest, CreateString1)
{
    OpcUaVariant variant = OpcUaVariant("Test");
    ASSERT_FALSE(variant.isBool());
    ASSERT_FALSE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_TRUE(variant.isString());
    ASSERT_FALSE(variant.isNodeId());
    ASSERT_STREQ(variant.toString().c_str(), "Test");
}

TEST_F(OpcUaVariantTest, CreateNodeId)
{
    OpcUaVariant variant(OpcUaNodeId(1, "Test"));

    ASSERT_FALSE(variant.isBool());
    ASSERT_FALSE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_TRUE(variant.isNodeId());

    ASSERT_EQ(variant.toNodeId(), OpcUaNodeId(1, "Test"));
}

TEST_F(OpcUaVariantTest, CopyConstructor)
{
    UA_Variant* ua_variant = UA_Variant_new();
    double value = 5;
    UA_Variant_setScalarCopy(ua_variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]);

    OpcUaVariant variant(*ua_variant);
    variant.setValue(*ua_variant);

    ASSERT_FALSE(variant.isBool());
    ASSERT_TRUE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_EQ(variant.toDouble(), 5);
    UA_Variant_delete(ua_variant);
}

TEST_F(OpcUaVariantTest, MoveConstructor)
{
    UA_Variant ua_variant;
    double value = 5;
    UA_Variant_setScalarCopy(&ua_variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]);

    OpcUaVariant variant(std::move(ua_variant));
    ASSERT_FALSE(variant.isBool());
    ASSERT_TRUE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_EQ(variant.toDouble(), 5);

    ASSERT_TRUE(UA_Variant_isEmpty(&ua_variant));
}

TEST_F(OpcUaVariantTest, MoveAssignment)
{
    OpcUaVariant variant("Hi");
    variant = OpcUaVariant("Test");
    ASSERT_TRUE(variant.isString());
    ASSERT_STREQ(variant.toString().c_str(), "Test");
}

TEST_F(OpcUaVariantTest, SetValueCopy)
{
    UA_Variant ua_variant;
    double value = 5;
    UA_Variant_setScalarCopy(&ua_variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]);

    OpcUaVariant variant;
    variant.setValue(ua_variant);
    ASSERT_FALSE(variant.isBool());
    ASSERT_TRUE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_EQ(variant.toDouble(), 5);

    UA_Variant_clear(&ua_variant);
}

TEST_F(OpcUaVariantTest, SetValueMove)
{
    UA_Variant ua_variant;
    double value = 5;
    UA_Variant_setScalarCopy(&ua_variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]);

    OpcUaVariant variant;
    variant.setValue(std::move(ua_variant));
    ASSERT_FALSE(variant.isBool());
    ASSERT_TRUE(variant.isDouble());
    ASSERT_FALSE(variant.isInteger());
    ASSERT_FALSE(variant.isString());
    ASSERT_EQ(variant.toDouble(), 5);

    ASSERT_TRUE(UA_Variant_isEmpty(&ua_variant));
}

TEST_F(OpcUaVariantTest, CopyConstructOperator)
{
    OpcUaVariant variant(int64_t(5));
    OpcUaVariant variant1 = variant;

    ASSERT_TRUE(variant.isInteger());
    ASSERT_EQ(variant.toInteger(), 5);

    ASSERT_TRUE(variant1.isInteger());
    ASSERT_EQ(variant1.toInteger(), 5);
}

TEST_F(OpcUaVariantTest, AssignmentOperator)
{
    OpcUaVariant variant(int64_t(5));
    OpcUaVariant variant1;
    variant1 = variant;

    ASSERT_TRUE(variant.isInteger());
    ASSERT_EQ(variant.toInteger(), 5);

    ASSERT_TRUE(variant1.isInteger());
    ASSERT_EQ(variant1.toInteger(), 5);
}

TEST_F(OpcUaVariantTest, ReadScalar)
{
    OpcUaVariant variant(static_cast<int64_t>(5));

    ASSERT_EQ(variant.readScalar<int64_t>(), 5);

    ASSERT_THROW(variant.readScalar<int32_t>(), std::runtime_error);
}

TEST_F(OpcUaVariantTest, SetScalar)
{
    OpcUaVariant variant;

    ASSERT_NO_THROW(variant.setScalar<int64_t>(5));

    ASSERT_EQ(variant.readScalar<int64_t>(), 5);
}

END_NAMESPACE_OPENDAQ_OPCUA
