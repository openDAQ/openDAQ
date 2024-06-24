#include <coreobjects/unit_factory.h>
#include <coreobjects/exceptions.h>
#include <gtest/gtest.h>

using UnitTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(UnitTest, UnitSetGet)
{
    const auto unit = UnitBuilder()
                      .setName("Unit")
                      .setId(100)
                      .setQuantity("Quantity")
                      .setSymbol("Symbol")
                      .build();

    ASSERT_EQ(unit.getName(), "Unit");
    ASSERT_EQ(unit.getId(), 100);
    ASSERT_EQ(unit.getQuantity(), "Quantity");
    ASSERT_EQ(unit.getSymbol(), "Symbol");
}

TEST_F(UnitTest, UnitNonBuilderFactory)
{
    auto unit = Unit("Symbol", 100, "Unit", "Quantity");

    ASSERT_EQ(unit.getName(), "Unit");
    ASSERT_EQ(unit.getId(), 100);
    ASSERT_EQ(unit.getQuantity(), "Quantity");
    ASSERT_EQ(unit.getSymbol(), "Symbol");
}

TEST_F(UnitTest, UnitCopyFactory)
{
    auto unit = Unit("Symbol", 100, "Unit", "Quantity");
    auto unitCopy = UnitBuilderCopy(unit).build();

    ASSERT_EQ(unitCopy.getName(), "Unit");
    ASSERT_EQ(unitCopy.getId(), 100);
    ASSERT_EQ(unitCopy.getQuantity(), "Quantity");
    ASSERT_EQ(unitCopy.getSymbol(), "Symbol");
}

TEST_F(UnitTest, Equals)
{
    auto unit = Unit("Symbol", 100, "Unit", "Quantity");
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
    auto serString = String(R"({"__type":"Unit","Symbol":"Symbol","Id":100,"Name":"Unit","Quantity":"Quantity"})");
    auto unit = Unit("Symbol", 100, "Unit", "Quantity");

    auto serializer = JsonSerializer();
    unit.serialize(serializer);

    ASSERT_EQ(serializer.getOutput(), serString);
}

TEST_F(UnitTest, Deserialize)
{
    auto serString = String(R"({"__type":"Unit","Symbol":"Symbol","Id":100,"Name":"Unit","Quantity":"Quantity"})");
    auto unit = Unit("Symbol", 100, "Unit", "Quantity");

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
                                .setName("Unit")
                                .setId(100)
                                .setQuantity("Quantity")
                                .setSymbol("Symbol");
    
    ASSERT_EQ(unitBuilder.getName(), "Unit");
    ASSERT_EQ(unitBuilder.getId(), 100);
    ASSERT_EQ(unitBuilder.getQuantity(), "Quantity");
    ASSERT_EQ(unitBuilder.getSymbol(), "Symbol");
}

TEST_F(UnitTest, UnitCreateFactory)
{
    const auto unitBuilder = UnitBuilder()
                                .setName("Unit")
                                .setId(100)
                                .setQuantity("Quantity")
                                .setSymbol("Symbol");
    const auto unit = UnitFromBuilder(unitBuilder);

    ASSERT_EQ(unit.getName(), "Unit");
    ASSERT_EQ(unit.getId(), 100);
    ASSERT_EQ(unit.getQuantity(), "Quantity");
    ASSERT_EQ(unit.getSymbol(), "Symbol");
}


END_NAMESPACE_OPENDAQ
