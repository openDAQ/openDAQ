#include <coretypes/coretypes.h>
#include <gtest/gtest.h>

using namespace daq;

using TestCallbackFactoryDeserialization = testing::Test;

TEST_F(TestCallbackFactoryDeserialization, RatioFactory)
{
    const auto serializer = JsonSerializer();
    const auto deserializer = JsonDeserializer();
    const auto ratio = Ratio(1, 2);

    ratio.serialize(serializer);
    const auto serializedJson = serializer.getOutput();

    bool ratioConstructed = false;

    const RatioPtr ptr = deserializer.deserialize(
        serializedJson,
        nullptr,
        [&ratioConstructed](const StringPtr&, const SerializedObjectPtr& serObj, const BaseObjectPtr&, const FunctionPtr&) -> BaseObjectPtr
    {
        const Int num = serObj.readInt("num");
        const Int den = serObj.readInt("den");
        ratioConstructed = true;
        return Ratio(num, den);
    });

    ASSERT_EQ(ptr.getNumerator(), ratio.getNumerator());
    ASSERT_EQ(ptr.getDenominator(), ratio.getDenominator());
    ASSERT_TRUE(ratioConstructed);
}

TEST_F(TestCallbackFactoryDeserialization, RatioInList)
{
    const auto serializer = JsonSerializer();
    const auto deserializer = JsonDeserializer();
    const auto list = List<IRatio>(Ratio(1, 2));

    list.serialize(serializer);
    const auto serializedJson = serializer.getOutput();

    bool ratioConstructed = false;
    const ListPtr<IRatio> ptr = deserializer.deserialize(serializedJson,
                                                  nullptr,
                                                  [&ratioConstructed](const StringPtr& typeId,
                                                     const SerializedObjectPtr& serObj,
                                                     const BaseObjectPtr&,
                                                     const FunctionPtr&) -> BaseObjectPtr
                                                  {
                                                         if (typeId == "Ratio")
                                                         {
                                                             const Int num = serObj.readInt("num");
                                                             const Int den = serObj.readInt("den");
                                                             ratioConstructed = true;
                                                             return Ratio(num, den);
                                                         }
                                                         return nullptr;
                                                  });

    ASSERT_EQ(ptr, list);
    ASSERT_TRUE(ratioConstructed);
}
