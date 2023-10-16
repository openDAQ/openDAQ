#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

using StringObjectTest = testing::Test;

TEST_F(StringObjectTest, Basic)
{
    auto string1 = String("Test");

    ConstCharPtr str;
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(string1->getCharPtr(&str)));
    ASSERT_EQ(strcmp(str, "Test"), 0);

    size_t size;
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(string1->getLength(&size)));
    ASSERT_EQ(size, 4u);
}

TEST_F(StringObjectTest, Empty)
{
    auto string2 = String(nullptr);

    ConstCharPtr str;
    string2->getCharPtr(&str);
    ASSERT_EQ(str, nullptr);

    size_t size;
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(string2->getLength(&size)));
    ASSERT_EQ(size, 0u);
}

TEST_F(StringObjectTest, Equality)
{
    auto string1 = String("Test12");
    auto string2 = String("Test12");
    auto string3 = String("Test3");
    auto string4 = String(nullptr);
    auto string5 = String(nullptr);

    Bool eq{false};
    string1->equals(string2, &eq);
    ASSERT_TRUE(eq);

    string1->equals(string3, &eq);
    ASSERT_FALSE(eq);

    string1->equals(string4, &eq);
    ASSERT_FALSE(eq);

    string4->equals(string5, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(StringObjectTest, Hashing)
{
    auto string1 = String("Test2");
    auto string2 = String("Test1");

    size_t hashCode1;
    size_t hashCode2;
    string1->getHashCode(&hashCode1);
    ASSERT_NE(hashCode1, 0u);
    string2->getHashCode(&hashCode2);
    ASSERT_NE(hashCode2, 0u);
    ASSERT_NE(hashCode1, hashCode2);

    auto string3 = String(nullptr);
    string3->getHashCode(&hashCode1);

    ASSERT_EQ(hashCode1, 0u);
}

BEGIN_NAMESPACE_OPENDAQ

DECLARE_OPENDAQ_INTERFACE(ITestObject, IBaseObject)
{
};

END_NAMESPACE_OPENDAQ

TEST_F(StringObjectTest, CastToPtr)
{
    auto string1 = String("Test2");
    auto string2 = PTR_CAST(string1, IBaseObject);
    ASSERT_THROW(auto string3 = PTR_CAST(string1, ITestObject), NoInterfaceException);
}

TEST_F(StringObjectTest, Conversion)
{
    auto string1 = String("1");
    auto conv = PTR_CAST(string1, IConvertible);
    Int valInt;
    ASSERT_EQ(conv->toInt(&valInt), OPENDAQ_SUCCESS);
    ASSERT_EQ(valInt, 1);

    Float valFloat;
    ASSERT_EQ(conv->toFloat(&valFloat), OPENDAQ_SUCCESS);
    ASSERT_EQ(valFloat, 1);

    auto conv1 = PTR_CAST(String("a"), IConvertible);
    ASSERT_EQ(conv1->toInt(&valInt), OPENDAQ_ERR_CONVERSIONFAILED);
    ASSERT_EQ(conv1->toFloat(&valFloat), OPENDAQ_ERR_CONVERSIONFAILED);
}

TEST_F(StringObjectTest, BoolConversion)
{
    ObjectPtr<IConvertible> convBool = PTR_CAST(String("True"), IConvertible);
    Bool valBool;
    ASSERT_EQ(convBool->toBool(&valBool), OPENDAQ_SUCCESS);
    ASSERT_EQ(valBool, True);

    convBool = PTR_CAST(String("False"), IConvertible);
    ASSERT_EQ(convBool->toBool(&valBool), OPENDAQ_SUCCESS);
    ASSERT_EQ(valBool, False);

    convBool = PTR_CAST(String("1"), IConvertible);
    ASSERT_EQ(convBool->toBool(&valBool), OPENDAQ_SUCCESS);
    ASSERT_EQ(valBool, True);

    convBool = PTR_CAST(String("0"), IConvertible);
    ASSERT_EQ(convBool->toBool(&valBool), OPENDAQ_SUCCESS);
    ASSERT_EQ(valBool, False);

    convBool = PTR_CAST(String("axy"), IConvertible);
    ASSERT_EQ(convBool->toBool(&valBool), OPENDAQ_SUCCESS);
    ASSERT_EQ(valBool, False);
}

TEST_F(StringObjectTest, Cast)
{
    auto string1 = String("1");

    Int valInt = string1;
    ASSERT_EQ(valInt, 1);

    Float valFloat = string1;
    ASSERT_EQ(valFloat, 1.0);

    Bool valBool = string1;
    ASSERT_EQ(valBool, True);
}

TEST_F(StringObjectTest, Unicode)
{
    auto stringObj = StringPtr(L"Test");
    std::wstring str = stringObj;

    ASSERT_EQ(str, std::wstring(L"Test"));
}

TEST_F(StringObjectTest, SerializeId)
{
    auto stringObj = String("Test");

    ASSERT_THROW(stringObj.getSerializeId(), NotImplementedException);
}

TEST_F(StringObjectTest, CoreType)
{
    auto stringObj = String("Test");
    auto coreType = stringObj.getCoreType();

    ASSERT_EQ(coreType, ctString);
}

TEST_F(StringObjectTest, ToStdStringIfObjectNull)
{
    StringPtr ptr;

    ASSERT_THROW(auto str = ptr.toStdString(), InvalidParameterException);
}

TEST_F(StringObjectTest, StringPtrToStdString)
{
    StringPtr ptr = "test";
    std::string str = ptr;

    ASSERT_EQ(str, "test");
}

static constexpr char ConstexprLiteral[] = "SomeTest";

TEST_F(StringObjectTest, ConcatenateConstexprCharLeft)
{
    StringPtr ptr = "Left";
    std::string str = ConstexprLiteral + ptr;
}

TEST_F(StringObjectTest, ConcatenateConstexprCharRight)
{
    StringPtr ptr = "Left";
    std::string str = ptr + ConstexprLiteral;
}

TEST_F(StringObjectTest, ConcatenateLiteralLeft)
{
    StringPtr ptr = "Left";
    std::string str = ptr + "SomeTest";
}

TEST_F(StringObjectTest, ConcatenateLiteralRight)
{
    StringPtr ptr = "Right";

    std::string str = "SomeTest" + ptr;
}

TEST_F(StringObjectTest, Inspectable)
{
    StringPtr obj = "Right";

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IString::Id);
}

TEST_F(StringObjectTest, ImplementationName)
{
    StringPtr obj = "Right";

    StringPtr className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    ASSERT_EQ(className, "daq::StringImpl");
}
