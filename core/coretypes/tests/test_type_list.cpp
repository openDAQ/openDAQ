#include <gtest/gtest.h>
#include <coretypes/arguments.h>
#include <coretypes/intfs.h>
#include <coretypes/coretype.h>
#include <coretypes/serializable.h>
#include <coretypes/updatable.h>

using namespace daq;

using TypeListTest = testing::Test;
using TypeList = Args<int, bool, float>;

TEST_F(TypeListTest, Create)
{
    ASSERT_TRUE((std::is_same_v<int, TypeList::Head>));
    ASSERT_TRUE((std::is_same_v<bool, TypeList::Tail::Head>));
    ASSERT_TRUE((std::is_same_v<float, TypeList::Tail::Tail::Head>));
    ASSERT_TRUE((std::is_same_v<Details::EndTag, TypeList::Tail::Tail::Tail>));
}

TEST_F(TypeListTest, Arity)
{
    using Arg0 = Args<>;
    using Arg1 = Args<int>;
    using Arg2 = Args<int, bool>;
    using Arg3 = Args<int, bool, float>;
    using Arg4 = Args<int, bool, float, char>;

    ASSERT_EQ(Arg0::Arity(), 0u);
    ASSERT_EQ(Arg1::Arity(), 1u);
    ASSERT_EQ(Arg2::Arity(), 2u);
    ASSERT_EQ(Arg3::Arity(), 3u);
    ASSERT_EQ(Arg4::Arity(), 4u);
}

TEST_F(TypeListTest, AtIndex)
{
    using FirstType = Meta::TypeAt<TypeList, 0>::Result;
    using SecondType = Meta::TypeAt<TypeList, 1>::Result;
    using ThirdType = Meta::TypeAt<TypeList, 2>::Result;

    ASSERT_TRUE((std::is_same_v<int, FirstType>));
    ASSERT_TRUE((std::is_same_v<bool, SecondType>));
    ASSERT_TRUE((std::is_same_v<float, ThirdType>));
}

TEST_F(TypeListTest, EqualTo)
{
    using TypeList2 = Args<int, bool, float>;

    ASSERT_TRUE((Meta::AreArgumentsEqual<TypeList, TypeList2>::Value));
}

TEST_F(TypeListTest, NotEqualSize)
{
    using TypeList2 = Args<int, bool, float, char>;

    ASSERT_FALSE((Meta::AreArgumentsEqual<TypeList, TypeList2>::Value));
}

TEST_F(TypeListTest, NotEqualArguments)
{
    using TypeList2 = Args<float, bool, int>;

    ASSERT_FALSE((Meta::AreArgumentsEqual<TypeList, TypeList2>::Value));
}

TEST_F(TypeListTest, HasType)
{
    ASSERT_TRUE((Meta::HasArgumentWithType<TypeList, int>::Value));
    ASSERT_TRUE((Meta::HasArgumentWithType<TypeList, bool>::Value));
    ASSERT_TRUE((Meta::HasArgumentWithType<TypeList, float>::Value));
}

TEST_F(TypeListTest, HasTypeFalse)
{
    ASSERT_FALSE((Meta::HasArgumentWithType<TypeList, double>::Value));
}

TEST_F(TypeListTest, AddToEmptyArgs)
{
    using EmptyArgs = Args<>;
    using Added = typename Meta::AddType<int, EmptyArgs>::Args;

    using Expected = Args<int>;

    ASSERT_TRUE((std::is_same_v<Expected, Added>));
}

TEST_F(TypeListTest, AddOneType)
{
    using Add1 = Meta::AddType<char, TypeList>::Args;
    using Expected = Args<int, bool, float, char>;

    ASSERT_TRUE((std::is_same_v<Add1, Expected>));
}

TEST_F(TypeListTest, AddTwoTypes)
{
    using Add2 = Meta::AddTypes<TypeList, char, double>::Args;
    using Expected = Args<int, bool, float, char, double>;

    ASSERT_TRUE((std::is_same_v<Add2, Expected>));
}

TEST_F(TypeListTest, AddThreeTypes)
{
    using Add3 = Meta::AddTypes<TypeList, char, double, std::nullptr_t>::Args;
    using Expected = Args<int, bool, float, char, double, std::nullptr_t>;

    ASSERT_TRUE((std::is_same_v<Add3, Expected>));
}

TEST_F(TypeListTest, AddZeroTypes)
{
    using Add2 = Meta::AddTypes<TypeList>::Args;
    using Expected = Args<int, bool, float>;

    ASSERT_TRUE((std::is_same_v<Add2, Expected>));
}

TEST_F(TypeListTest, PrependToEmptyArgs)
{
    using EmptyArgs = Args<>;
    using Prepend = typename Meta::PrependType<int, EmptyArgs>::Args;

    using Expected = Args<int>;

    ASSERT_TRUE((std::is_same_v<Expected, Prepend>));
}

TEST_F(TypeListTest, PrependOneType)
{
    using Prepend1 = Meta::PrependType<char, TypeList>::Args;
    using Expected = Args<char, int, bool, float>;

    ASSERT_TRUE((std::is_same_v<Prepend1, Expected>));
}

TEST_F(TypeListTest, PrependTwoTypes)
{
    using Prepend2 = Meta::PrependTypes<TypeList, char, double>::Args;
    using Expected = Args<char, double, int, bool, float>;

    ASSERT_TRUE((std::is_same_v<Prepend2, Expected>));
}

TEST_F(TypeListTest, PrependThreeTypes)
{
    using Prepend3 = Meta::PrependTypes<TypeList, char, double, std::nullptr_t>::Args;
    using Expected = Args<char, double, std::nullptr_t, int, bool, float>;

    ASSERT_TRUE((std::is_same_v<Prepend3, Expected>));
}

TEST_F(TypeListTest, RemoveOne)
{
    using RemovedFirst = Meta::RemoveOneOf<int, TypeList>::Folded;
    using Expected1 = Args<bool, float>;
    ASSERT_TRUE((std::is_same_v<Expected1, RemovedFirst>));

    using RemoveSecond = Meta::RemoveOneOf<bool, TypeList>::Folded;
    using Expected2 = Args<int, float>;
    ASSERT_TRUE((std::is_same_v<Expected2, RemoveSecond>));

    using RemovedThird = Meta::RemoveOneOf<float, TypeList>::Folded;
    using Expected3 = Args<int, bool>;

    ASSERT_TRUE((std::is_same_v<Expected3, RemovedThird>));
}

// clang-format off

TEST_F(TypeListTest, RemoveAllOf)
{
    using TypeListMultiple = Args<int, bool, float, int, char, double, int>;
    using Expected =         Args<     bool, float,      char, double>;

    using RemovedAllInts = Meta::RemoveAllOf<int, TypeListMultiple>::Folded;

    ASSERT_TRUE((std::is_same_v<Expected, RemovedAllInts>));
}

TEST_F(TypeListTest, Unique)
{
    using TypeListMultiple = Args<int, bool, float, int, double, bool, double, int, char, float>;
    using Expected =         Args<int, bool, float,      double,                    char>;

    using Unique = Meta::UniqueTypes<TypeListMultiple>::Args;

    ASSERT_TRUE((std::is_same_v<Expected, Unique>));
}

// clang-format on

TEST_F(TypeListTest, ReverseOneArg)
{
    using TypeList1 = Args<int>;

    using Reverse1 = Meta::ReverseTypes<TypeList1>::Args;
    using Expected1 = TypeList1;
    ASSERT_TRUE((std::is_same_v<Expected1, Reverse1>));
}

TEST_F(TypeListTest, ReverseTwoArgs)
{
    using TypeList2 = Args<int, bool>;

    using Reverse2 = Meta::ReverseTypes<TypeList2>::Args;
    using Expected2 = Args<bool, int>;
    ASSERT_TRUE((std::is_same_v<Expected2, Reverse2>));
}

TEST_F(TypeListTest, ReverseThreeArgs)
{
    using TypeList3 = Args<int, bool, float>;

    using Reverse3 = Meta::ReverseTypes<TypeList3>::Args;
    using Expected3 = Args<float, bool, int>;
    ASSERT_TRUE((std::is_same_v<Expected3, Reverse3>));
}

TEST_F(TypeListTest, ConcatTwoArgs)
{
    using TypeList1 = Args<int, bool, char>;
    using TypeList2 = Args<float, double, unsigned>;

    using Concatinated = Details::ConcatTwoArgs<TypeList1, TypeList2>::Args;
    using Expected = Args<int, bool, char, float, double, unsigned>;

    ASSERT_TRUE((std::is_same_v<Expected, Concatinated>));
}

TEST_F(TypeListTest, ConcatArgs)
{
    using TypeList1 = Args<int, bool>;
    using TypeList2 = Args<float, double>;
    using TypeList3 = Args<char, unsigned>;

    using Concatinated = Details::ConcatArgs<TypeList1, TypeList2, TypeList3>::Args;
    using Expected = Args<int, bool, float, double, char, unsigned>;

    ASSERT_TRUE((std::is_same_v<Expected, Concatinated>));
}

TEST_F(TypeListTest, WrapTypes)
{
    using BaseTypes = Args<IBaseObject>;
    using Test = typename Meta::WrapTypesWith<DiscoverOnly, BaseTypes>::Wrapped;

    ASSERT_TRUE((std::is_same_v<Test::Head, DiscoverOnly<IBaseObject>>));
}

template <typename...>
struct Variadic
{
};

TEST_F(TypeListTest, Fold)
{
    using InterfacesList = Args<ICoreType, ISerializable, IUpdatable, IBaseObject>;
    using Fold = Meta::FoldType<InterfacesList, Variadic>::Folded;

    using Expected = Variadic<ICoreType, ISerializable, IUpdatable, IBaseObject>;

    ASSERT_TRUE((std::is_same_v<Expected, Fold>));
}

TEST_F(TypeListTest, IndexOf)
{
    using Types = Args<int, bool, float>;

    int result = Meta::IndexOf<Types, bool>::Index;
    ASSERT_EQ(result, 1);
}

TEST_F(TypeListTest, Flatten)
{
    using NestedArgs = Args<Args<IBaseObject>, Args<IUpdatable, ISerializable>>;
    using Flat = Meta::Flatten<NestedArgs>::Args;

    using Expected = Args<IBaseObject, IUpdatable, ISerializable>;

    ASSERT_TRUE((std::is_same_v<Expected, Flat>));
}
