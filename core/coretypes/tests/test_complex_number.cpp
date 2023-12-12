#include <complex>
#include <testutils/testutils.h>
#include <coretypes/complex_number_factory.h>

using namespace daq;

using ComplexNumberTest = testing::Test;

TEST_F(ComplexNumberTest, Basic)
{
    auto obj = ComplexNumber(5.0, 2.0);
    ASSERT_EQ(obj.getReal(), 5.0);
    ASSERT_EQ(obj.getImaginary(), 2.0);
}

TEST_F(ComplexNumberTest, GetValue)
{
    auto obj = ComplexNumber(5.0, 2.0);
    ASSERT_EQ(obj.getValue(), ComplexFloat64(5.0, 2.0));
}

TEST_F(ComplexNumberTest, EqualsValue)
{
    auto obj = ComplexNumber(5.0, 2.0);
    ASSERT_TRUE(obj.equalsValue(ComplexFloat64(5.0, 2.0)));
    ASSERT_FALSE(obj.equalsValue(ComplexFloat64(1.0, 1.0)));
}

TEST_F(ComplexNumberTest, Equality)
{
    auto obj1 = ComplexNumber(3.0, 30.0);
    auto obj2 = ComplexNumber(3.0, 30.0);
    auto obj3 = ComplexNumber(4.0, 40.0);

    Bool eq{false};

    obj1->equals(obj1, &eq);
    ASSERT_TRUE(eq);

    obj1->equals(obj2, &eq);
    ASSERT_TRUE(eq);

    obj1->equals(obj3, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(ComplexNumberTest, Hashing)
{
    auto obj1 = ComplexNumber(3.0, -1.0);
    auto obj2 = ComplexNumber(3.0, 5.0);

    size_t hash1 = 0;
    size_t hash2 = 0;
    obj1->getHashCode(&hash1);
    ASSERT_NE(hash1, 0u);
    obj2->getHashCode(&hash2);
    ASSERT_NE(hash2, 0u);
    ASSERT_NE(hash1, hash2);
}

TEST_F(ComplexNumberTest, CastToPtr)
{
    auto obj1 = ComplexNumber(3.0, -1.0);
    auto obj2 = PTR_CAST(obj1, IBaseObject);
}

TEST_F(ComplexNumberTest, ToString)
{
    auto obj = ComplexNumber(1.0, -0.5);
    const std::string str = obj.toString();
    ASSERT_EQ(str, "(1, -0.5)");
}

TEST_F(ComplexNumberTest, CoreType)
{
    auto obj = ComplexNumber(1.0, 0.0);
    enum CoreType coreType = obj.getCoreType();
    ASSERT_EQ(coreType, ctComplexNumber);
}

TEST_F(ComplexNumberTest, List)
{
    auto v1 = ComplexFloat64(1, 0);
    auto v2 = ComplexFloat64(2, 0);
    auto v3 = ComplexFloat64(3, 0);

    auto list = List<IComplexNumber>(v1, v2);
    list.pushBack(v3);

    Bool eq{false};
    list[0]->equalsValue(v1, &eq);
    ASSERT_TRUE(eq);

    list[1]->equalsValue(v2, &eq);
    ASSERT_TRUE(eq);

    list[2]->equalsValue(v3, &eq);
    ASSERT_TRUE(eq);

    list[2]->equalsValue(v1, &eq);
    ASSERT_FALSE(eq);
}

TEST_F(ComplexNumberTest, ListCast)
{
    auto v1 = ComplexFloat64(1, 2);

    auto list = List<IBaseObject>(v1);
    ListPtr<IComplexNumber> complexList = List<IComplexNumber>();
    complexList.pushBack(ComplexNumberPtr(list[0]));

    Bool eq{false};
    complexList[0]->equalsValue(v1, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(ComplexNumberTest, List32To64)
{
    auto v32 = ComplexFloat32(1.5, 2.5);
    auto v64 = ComplexFloat64(3.5, 4.5);

    ListPtr<IComplexNumber> list = List<IComplexNumber>();
    list.pushBack(v32);
    list.pushBack(v64);

    Bool eq{false};
    list[0]->equalsValue(v32, &eq);
    ASSERT_TRUE(eq);

    eq = false;
    list[1]->equalsValue(v64, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(ComplexNumberTest, IsSerializable)
{
    auto complex = ComplexNumber();
    ASSERT_TRUE(complex.template supportsInterface<ISerializable>());
}

TEST_F(ComplexNumberTest, Serialize)
{
    const std::string expected = R"({"__type":"ComplexNumber","real":1.5,"imaginary":2.5})";

    auto deserializer = JsonDeserializer();
    auto complex = deserializer.deserialize(expected);

    auto serializer = JsonSerializer();
    complex.serialize(serializer);
    const auto output = serializer.getOutput();

    ASSERT_EQ(output, expected);
}

TEST_F(ComplexNumberTest, Inspectable)
{
    auto obj = ComplexNumber();

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IComplexNumber::Id);
}

TEST_F(ComplexNumberTest, ImplementationName)
{
    auto obj = ComplexNumber();

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::ComplexNumberImpl");
}

TEST_F(ComplexNumberTest, StructType)
{
}

TEST_F(ComplexNumberTest, StructFields)
{
}

TEST_F(ComplexNumberTest, StructNames)
{
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IComplexNumber", "daq");

TEST_F(ComplexNumberTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IComplexNumber::Id);
}

TEST_F(ComplexNumberTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IComplexNumber>(), "{FB9C2303-3E0E-5213-8C9B-3BD39B EA61C}");
}
