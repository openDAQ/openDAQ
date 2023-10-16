#include <opendaq/range_factory.h>
#include <opendaq/signal_exceptions.h>
#include <gtest/gtest.h>
#include <coretypes/json_serializer.h>
#include <coretypes/json_deserializer_factory.h>

using RangeTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(RangeTest, CreateIntRange)
{
    const auto range = Range(2, 4);
    ASSERT_EQ(range.getLowValue(), 2);
    ASSERT_EQ(range.getHighValue(), 4);
}

TEST_F(RangeTest, CreateFloatRange)
{
    const auto range = Range(2.2, 4.4);
    ASSERT_EQ(range.getLowValue(), 2.2);
    ASSERT_EQ(range.getHighValue(), 4.4);
}

TEST_F(RangeTest, SerializeDeserializeIntRange)
{
    const auto range = Range(2, 4);
    auto serializer = JsonSerializer(False);
    range.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto range1 = deserializer.deserialize(serialized.toStdString()).asPtr<IRange>();

    ASSERT_EQ(range1.getLowValue(), 2);
    ASSERT_EQ(range1.getHighValue(), 4);
}

TEST_F(RangeTest, SerializeDeserializeFloatRange)
{
    const auto range = Range(2.2, 4.4);
    auto serializer = JsonSerializer(False);
    range.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto range1 = deserializer.deserialize(serialized.toStdString()).asPtr<IRange>();

    ASSERT_EQ(range1.getLowValue(), 2.2);
    ASSERT_EQ(range1.getHighValue(), 4.4);
}

TEST_F(RangeTest, StructType)
{
    const auto structType = RangeStructType();
    const StructPtr structPtr = Range(1, 2);
    ASSERT_EQ(structPtr.getStructType(), structType);
}

TEST_F(RangeTest, StructFields)
{
    const StructPtr structPtr = Range(1, 2);
    ASSERT_EQ(structPtr.get("lowValue"), 1);
    ASSERT_EQ(structPtr.get("highValue"), 2);
}

TEST_F(RangeTest, StructNames)
{
    const auto structType = RangeStructType();
    const StructPtr structPtr = Range(1, 2);
    ASSERT_EQ(structPtr.getFieldNames(), structType.getFieldNames());
}

END_NAMESPACE_OPENDAQ
