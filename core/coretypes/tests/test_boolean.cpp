#include <gtest/gtest.h>
#include <coretypes/boolean_factory.h>
#include <coretypes/inspectable_ptr.h>

using namespace daq;

using BooleanTest = testing::Test;

TEST_F(BooleanTest, Basic)
{
    auto boolObj = Boolean(True);
    Bool boolVal;
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(boolObj->getValue(&boolVal)));
    ASSERT_EQ(boolVal, True);
}

TEST_F(BooleanTest, Equality)
{
    auto boolObj1 = Boolean(True);
    auto boolObj2 = Boolean(True);
    auto boolObj3 = Boolean(False);

    Bool eq{false};
    boolObj1->equals(boolObj1, &eq);
    ASSERT_TRUE(eq);

    boolObj1->equals(boolObj2, &eq);
    ASSERT_TRUE(eq);

    boolObj1->equals(boolObj3, &eq);
    ASSERT_FALSE(eq);

    boolObj1->equalsValue(True, &eq);
    ASSERT_TRUE(eq);
}

TEST_F(BooleanTest, Hashing)
{
    auto boolObj1 = Boolean(True);
    auto boolObj2 = Boolean(False);

    size_t hashCode1;
    size_t hashCode2;
    boolObj1->getHashCode(&hashCode1);
    ASSERT_NE(hashCode1, 0u);
    boolObj2->getHashCode(&hashCode2);
    ASSERT_EQ(hashCode2, 0u);
    ASSERT_NE(hashCode1, hashCode2);
}

TEST_F(BooleanTest, CastToPtr)
{
    auto boolObj1 = Boolean(True);
    auto boolObj2 = PTR_CAST(boolObj1, IBaseObject);
}

TEST_F(BooleanTest, Cast)
{
    auto boolObj = Boolean(True);

    Int valInt = boolObj;
    ASSERT_EQ(valInt, 1);

    Float valFloat = boolObj;
    ASSERT_EQ(valFloat, 1.0);

    Bool valBool = boolObj;
    ASSERT_EQ(valBool, True);

    std::string str = boolObj;
    ASSERT_EQ(str, "True");
}

TEST_F(BooleanTest, CoreType)
{
    auto boolObj = Boolean(True);
    auto coreType = boolObj.getCoreType();

    ASSERT_EQ(coreType, ctBool);
}

TEST_F(BooleanTest, CBoolConversionTrue)
{
    auto obj = ObjectPtr<IBaseObject>(true);
    ASSERT_TRUE(static_cast<bool>(obj));
}

TEST_F(BooleanTest, CBoolConversionFalse)
{
    auto obj = ObjectPtr<IBaseObject>(false);
    ASSERT_FALSE(static_cast<bool>(obj));
}

TEST_F(BooleanTest, InitializationFromFalse)
{
    auto obj = ObjectPtr<IBoolean>(False);
    ASSERT_FALSE(static_cast<bool>(obj));
}

TEST_F(BooleanTest, InitializationFromTrue)
{
    auto obj = ObjectPtr<IBoolean>(True);
    ASSERT_TRUE(static_cast<bool>(obj));
}

TEST_F(BooleanTest, InitializationFromBoolTrue)
{
    auto obj = ObjectPtr<IBoolean>(true);
    ASSERT_TRUE(static_cast<bool>(obj));
}

TEST_F(BooleanTest, InitializationFromBoolFalse)
{
    auto obj = ObjectPtr<IBoolean>(false);
    ASSERT_FALSE(static_cast<bool>(obj));
}

TEST_F(BooleanTest, BooleanToBoolFalse)
{
    auto booleanObj = Boolean(false);
    Bool boolean = booleanObj;

    ASSERT_FALSE(boolean);
}

TEST_F(BooleanTest, IsBoolTrue)
{
    Bool boolTrue = True;

    ASSERT_TRUE(IsTrue(boolTrue));
}

TEST_F(BooleanTest, IsBoolFalse)
{
    Bool boolFalse = False;

    ASSERT_FALSE(IsTrue(boolFalse));
}

TEST_F(BooleanTest, Inspectable)
{
    auto obj = Boolean(false);

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IBoolean::Id);
}

TEST_F(BooleanTest, ImplementationName)
{
    auto obj = Boolean(false);
    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();

    auto prefix = className.find("daq::OrdinalObjectImpl<unsigned char,");
    ASSERT_EQ(prefix, 0u);
}

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IBoolean", "daq");

TEST_F(BooleanTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IBoolean::Id);
}

TEST_F(BooleanTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IBoolean>(), "{9F20E31A-D0FB-5679-A188-4942B3FED6E2}");
}
