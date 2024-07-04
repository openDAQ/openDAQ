#include <opendaq/range_factory.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/signal_exceptions.h>
#include <gtest/gtest.h>
#include "opendaq/dimension_rule_factory.h"

using DimensionTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

//// IRange

TEST_F(DimensionTest, TestRangeGet)
{
    auto range = Range(Integer(5), Floating(5.1));
    ASSERT_EQ(range.getLowValue(), 5);
    ASSERT_DOUBLE_EQ(range.getHighValue(), 5.1);
}

TEST_F(DimensionTest, TestRangeInvalidBoundaries)
{
    ASSERT_THROW(Range(Integer(6), Floating(5.0)), RangeBoundariesInvalidException);
    ASSERT_THROW(Range(Floating(5.1), Integer(5)), RangeBoundariesInvalidException);
    ASSERT_NO_THROW(Range(Integer(5), Integer(5)));
}


//// IDimension

TEST_F(DimensionTest, DimensionGetSet)
{
    const auto dim = DimensionBuilder()
                     .setRule(LinearDimensionRule(10, 10, 100))
                     .setUnit(Unit("symbol", 10))
                     .build();

    ASSERT_EQ(dim.getRule().getParameters().get("start"), 10);
    ASSERT_EQ(dim.getUnit().getSymbol(), "symbol");
}

TEST_F(DimensionTest, DimensionGetSizeList)
{
    const auto dim = DimensionBuilder()
                     .setRule(ListDimensionRule(List<IInteger>(10, 20, 30, 40, 50)))
                     .build();

    ASSERT_EQ(dim.getSize(), static_cast<SizeT>(5));
}

TEST_F(DimensionTest, DimensionGetSizeLog)
{
    const auto dim = DimensionBuilder()
                     .setRule(LinearDimensionRule(10, 10, 5))
                     .build();
    
    ASSERT_EQ(dim.getSize(), static_cast<SizeT>(5));
}

TEST_F(DimensionTest, DimensionGetSizeLin)
{
    auto dim = DimensionBuilder().setRule(LogarithmicDimensionRule(10, 10, 5, 5)).build();
    
    ASSERT_EQ(dim.getSize(), static_cast<SizeT>(5));
}

TEST_F(DimensionTest, DimensionGetLabelsLin)
{
    auto dim = DimensionBuilder().setRule(LinearDimensionRule(4, 5, 10)).build();
    
    int val = 5;
    for (int label : dim.getLabels())
    {
        ASSERT_EQ(label, val);
        val += 4;
    }
}

TEST_F(DimensionTest, DimensionGetLabelsList)
{
    auto dim = DimensionBuilder().setRule(ListDimensionRule(List<IInteger>(10, 20, 30, 40, 50))).build();
    
    int val = 10;
    for (int label : dim.getLabels())
    {
        ASSERT_EQ(label, val);
        val += 10;
    }
}

TEST_F(DimensionTest, DimensionGetLabelsLog)
{
    auto dim = DimensionBuilder().setRule(LogarithmicDimensionRule(1, 0, 10, 10)).build();

    int val = 1;
    for (int label : dim.getLabels())
    {
        ASSERT_EQ(label, val);
        val *= 10;
    }
}

TEST_F(DimensionTest, DimensionFreezeIncompleteConfiguration)
{
    auto dim = DimensionBuilder();
    ASSERT_THROW(dim.build(), ConfigurationIncompleteException);
    dim.setRule(LogarithmicDimensionRule(1, 0, 10, 20));
    ASSERT_NO_THROW(dim.build());
}

TEST_F(DimensionTest, DimensionCopyTestList)
{
    auto dimList = Dimension(ListDimensionRule(List<INumber>(10, 20, 30)));
    auto dimListCopy = DimensionBuilderCopy(dimList).build();
    ASSERT_EQ(dimListCopy.getSize(), static_cast<size_t>(3));
}

TEST_F(DimensionTest, DimensionCopyTestLin)
{
    auto dimLin = Dimension(LinearDimensionRule(4, 5, 10));
    auto dimLinCopy = DimensionBuilderCopy(dimLin).build();

    ASSERT_EQ(dimLinCopy.getSize(), static_cast<size_t>(10));
}

TEST_F(DimensionTest, DimensionCopyTestLog)
{
    auto dimLog = Dimension(LogarithmicDimensionRule(1, 0, 10, 10));
    auto dimLogCopy = DimensionBuilderCopy(dimLog).build();
    
    ASSERT_EQ(dimLogCopy.getSize(), static_cast<size_t>(10));
}

TEST_F(DimensionTest, DimensionOtherTest)
{
    auto dim = DimensionBuilder().setRule(DimensionRule(DimensionRuleType::Other, Dict<IString, IBaseObject>())).build();

    ASSERT_THROW(dim.getLabels(), UnknownRuleTypeException);
    ASSERT_THROW(dim.getSize(), UnknownRuleTypeException);
}

TEST_F(DimensionTest, SerializeDeserialize)
{
    auto dim = Dimension(ListDimensionRule(List<INumber>(10, 20, 30)));

    auto serializer = JsonSerializer(False);
    dim.serialize(serializer);

    auto serialized = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    auto dim1 = deserializer.deserialize(serialized.toStdString()).asPtr<IDimension>();

    ASSERT_EQ(dim1, dim);
}

TEST_F(DimensionTest, StructType)
{
    const auto structType = DimensionStructType();
    const daq::StructPtr structPtr = daq::Dimension(LinearDimensionRule(10, 1 ,5));
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(DimensionTest, StructFields)
{
    const StructPtr structPtr = Dimension(LinearDimensionRule(10, 1 ,10), Unit("Hz"), "test");
    ASSERT_EQ(structPtr.get("Rule"), LinearDimensionRule(10, 1, 10));
    ASSERT_EQ(structPtr.get("Unit"), Unit("Hz"));
    ASSERT_EQ(structPtr.get("Name"),  "test");
}

TEST_F(DimensionTest, StructNames)
{
    const auto structType = DimensionStructType();
    const daq::StructPtr structPtr = Dimension(LinearDimensionRule(10, 1 ,10), Unit("Hz"), "test");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(DimensionTest, DimensionBuilderSetGet)
{
    const auto rule = LinearDimensionRule(10, 20, 10);
    const auto dimensionBuilder = DimensionBuilder()
                                    .setName("Dimension")
                                    .setUnit(Unit("s"))
                                    .setRule(rule);
    
    ASSERT_EQ(dimensionBuilder.getName(), "Dimension");
    ASSERT_EQ(dimensionBuilder.getUnit(), Unit("s"));
    ASSERT_EQ(dimensionBuilder.getRule(), rule);
}

TEST_F(DimensionTest, DimensionCreateFactory)
{
    const auto rule = LinearDimensionRule(10, 20, 10);
    const auto dimensionBuilder = DimensionBuilder()
                                    .setName("Dimension")
                                    .setUnit(Unit("s"))
                                    .setRule(rule);
    const auto dimension = DimensionFromBuilder(dimensionBuilder);

    ASSERT_EQ(dimension.getName(), "Dimension");
    ASSERT_EQ(dimension.getUnit(), Unit("s"));
    ASSERT_EQ(dimension.getRule(), rule);
}


END_NAMESPACE_OPENDAQ
