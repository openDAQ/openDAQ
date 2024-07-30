#include <coreobjects/unit_factory.h>
#include <coreobjects/exceptions.h>
#include <gtest/gtest.h>

using UnitTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(UnitTest, UnitSetGet)
{
    const auto unit = UnitBuilder()
                      .setName("unit")
                      .setId(100)
                      .setQuantity("quantity")
                      .setSymbol("symbol")
                      .build();

    ASSERT_EQ(unit.getName(), "unit");
    ASSERT_EQ(unit.getId(), 100);
    ASSERT_EQ(unit.getQuantity(), "quantity");
    ASSERT_EQ(unit.getSymbol(), "symbol");
}

TEST_F(UnitTest, UnitNonBuilderFactory)
{
    auto unit = Unit("symbol", 100, "unit", "quantity");

    ASSERT_EQ(unit.getName(), "unit");
    ASSERT_EQ(unit.getId(), 100);
    ASSERT_EQ(unit.getQuantity(), "quantity");
    ASSERT_EQ(unit.getSymbol(), "symbol");
}

TEST_F(UnitTest, UnitCopyFactory)
{
    auto unit = Unit("symbol", 100, "unit", "quantity");
    auto unitCopy = UnitBuilderCopy(unit).build();

    ASSERT_EQ(unitCopy.getName(), "unit");
    ASSERT_EQ(unitCopy.getId(), 100);
    ASSERT_EQ(unitCopy.getQuantity(), "quantity");
    ASSERT_EQ(unitCopy.getSymbol(), "symbol");
}

TEST_F(UnitTest, Equals)
{
    auto unit = Unit("symbol", 100, "unit", "quantity");
    auto unitCopy = UnitBuilderCopy(unit);

    Bool eq{false};
    unitCopy.build()->equals(unit, &eq);
    ASSERT_TRUE(eq);

    unitCopy.setName("changed " + unit.getName());

    unitCopy.build()->equals(unit, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(UnitTest, Serialize)
{
    auto serString = String(R"({"__type":"Unit","symbol":"symbol","id":100,"name":"unit","quantity":"quantity"})");
    auto unit = Unit("symbol", 100, "unit", "quantity");

    auto serializer = JsonSerializer();
    unit.serialize(serializer);

    ASSERT_EQ(serializer.getOutput(), serString);
}

TEST_F(UnitTest, Deserialize)
{
    auto serString = String(R"({"__type":"Unit","symbol":"symbol","id":100,"name":"unit","quantity":"quantity"})");
    auto unit = Unit("symbol", 100, "unit", "quantity");

    auto deserializer = JsonDeserializer();
    auto unit1 = deserializer.deserialize(serString);

    ASSERT_EQ(unit, unit1);
}

TEST_F(UnitTest, StructType)
{
    const auto structType = UnitStructType();
    const StructPtr structPtr = Unit("s");
    ASSERT_EQ(structType, structPtr.getStructType());
}

TEST_F(UnitTest, StructFields)
{
    const StructPtr structPtr = Unit("s",-1, "seconds", "time");

    ASSERT_EQ(structPtr.get("Symbol"), "s");
    ASSERT_EQ(structPtr.get("Name"), "seconds");
    ASSERT_EQ(structPtr.get("Id"), -1);
    ASSERT_EQ(structPtr.get("Quantity"), "time");
}

TEST_F(UnitTest, StructNames)
{
    const auto structType = UnitStructType();
    const StructPtr structPtr = Unit("s");
    ASSERT_EQ(structType.getFieldNames(), structPtr.getFieldNames());
}

TEST_F(UnitTest, UnitBuilderSetGet)
{
    const auto unitBuilder = UnitBuilder()
                                .setName("unit")
                                .setId(100)
                                .setQuantity("quantity")
                                .setSymbol("symbol");
    
    ASSERT_EQ(unitBuilder.getName(), "unit");
    ASSERT_EQ(unitBuilder.getId(), 100);
    ASSERT_EQ(unitBuilder.getQuantity(), "quantity");
    ASSERT_EQ(unitBuilder.getSymbol(), "symbol");
}

TEST_F(UnitTest, UnitCreateFactory)
{
    const auto unitBuilder = UnitBuilder()
                                .setName("unit")
                                .setId(100)
                                .setQuantity("quantity")
                                .setSymbol("symbol");
    const auto unit = UnitFromBuilder(unitBuilder);

    ASSERT_EQ(unit.getName(), "unit");
    ASSERT_EQ(unit.getId(), 100);
    ASSERT_EQ(unit.getQuantity(), "quantity");
    ASSERT_EQ(unit.getSymbol(), "symbol");
}


END_NAMESPACE_OPENDAQ
