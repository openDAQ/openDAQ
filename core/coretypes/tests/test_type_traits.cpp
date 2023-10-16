#include <gtest/gtest.h>
#include <coretypes/string_ptr.h>
#include <coretypes/constexpr_utils.h>

using TypeTraitsTest = testing::Test;

////////////////////////////////////////
///        Classes
////////////////////////////////////////

template <typename T>
class Foo
{
};

template <typename T, typename U>
class Bar
{
};

template <typename... T>
class FooBar
{
};

class NotTemplateClass
{
};

////////////////////////////////////////
///        Structs
////////////////////////////////////////

template <typename T>
struct FooStruct
{
};

template <typename T, typename U>
struct BarStruct
{
};

template <typename... T>
class FooBarStruct
{
};

struct NotTemplateStruct
{
};

using namespace daq;

//////////////////////////////////////////////////////////
///                 Literal
//////////////////////////////////////////////////////////

TEST_F(TypeTraitsTest, IsNotTemplateLiteral)
{
    ASSERT_FALSE(IsTemplate<int>::value);
}

//////////////////////////////////////////////////////////
///                 Struct
//////////////////////////////////////////////////////////

TEST_F(TypeTraitsTest, IsNotTemplateStruct)
{
    ASSERT_FALSE(IsTemplate<NotTemplateStruct>::value);
}

TEST_F(TypeTraitsTest, IsTemplateStructOneParam)
{
    ASSERT_TRUE(IsTemplate<FooStruct<int>>::value);
}

TEST_F(TypeTraitsTest, IsTemplateStructTwoParam)
{
    ASSERT_TRUE((IsTemplate<BarStruct<int, float>>::value));
}

TEST_F(TypeTraitsTest, IsTemplateStructVariadic)
{
    ASSERT_TRUE((IsTemplate<FooBarStruct<int, float, bool, char>>::value));
}

//////////////////////////////////////////////////////////
///                 Class
//////////////////////////////////////////////////////////

TEST_F(TypeTraitsTest, IsNotTemplateClass)
{
    ASSERT_FALSE(IsTemplate<NotTemplateClass>::value);
}

TEST_F(TypeTraitsTest, IsTemplateClassOneParam)
{
    ASSERT_TRUE(IsTemplate<Foo<int>>::value);
}

TEST_F(TypeTraitsTest, IsTemplateClassTwoParam)
{
    ASSERT_TRUE((IsTemplate<Bar<int, float>>::value));
}

TEST_F(TypeTraitsTest, IsTemplateClassVariadic)
{
    ASSERT_TRUE((IsTemplate<FooBar<int, float, bool, char>>::value));
}

TEST_F(TypeTraitsTest, IsDerivedFromTemplate)
{
    ASSERT_TRUE((IsDerivedFromTemplate<StringPtr, ObjectPtr>::Value));
}
