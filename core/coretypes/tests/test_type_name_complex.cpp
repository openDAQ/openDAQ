#include <testutils/testutils.h>
#include <coretypes/common.h>
#include <coretypes/type_name.h>
#include <coretypes/constexpr_string.h>
#include <coretypes/constexpr_utils.h>

namespace daq
{
    template <std::size_t N>
    std::ostream& operator<<(std::ostream& os, const daq::ConstexprString<N>& bar)
    {
        return os << std::string(bar.data(), N);
    }
}

namespace Test::Foo::Bar
{
    class Testing
    {
    };

    template <typename T, typename U, typename V>
    class BarFoo
    {
    };

    template <typename T, typename U>
    class FooBar
    {
    };

    template <typename T>
    class Foo
    {
    };

    struct Record
    {
    };

    template <typename T>
    struct RecordStruct
    {
    };

    template <typename T, typename U>
    struct Struct
    {
    };

    template <typename T, typename U, typename V>
    struct StructRecord
    {
    };
}

class Testing
{
};

struct TestStruct
{
};

template <typename T, typename U, typename V>
class TemplateClass
{
};

template <typename T, typename U, typename V>
struct TemplateStruct
{
};

template <typename T>
class TemplatedGlobal
{
};

using TypeNameComplexTest = testing::Test;

using namespace daq;

////////////////////////////////////////////////////////////////////////////////////////
///              Global namespace struct
////////////////////////////////////////////////////////////////////////////////////////

using GlobalNamespaceStruct = TestStruct;

TEST_F(TypeNameComplexTest, GlobalNamespaceStruct)
{
    constexpr auto testingName = typeName<GlobalNamespaceStruct>();

    ASSERT_EQ(testingName, "TestStruct");
}

TEST_F(TypeNameComplexTest, GlobalNamespaceStructQualified)
{
    constexpr auto testingName = typeNameQualified<GlobalNamespaceStruct>();

    ASSERT_EQ(testingName, "TestStruct");
}

////////////////////////////////////////////////////////////////////////////////////////
///              Global namespace class
////////////////////////////////////////////////////////////////////////////////////////

using GlobalNamespaceClass = Testing;

TEST_F(TypeNameComplexTest, GlobalNamespaceClass)
{
    constexpr auto testingName = typeName<GlobalNamespaceClass>();

    ASSERT_EQ(testingName, "Testing");
}

TEST_F(TypeNameComplexTest, GlobalNamespaceClassQualified)
{
    auto testingName = typeNameQualified<GlobalNamespaceClass>();

    ASSERT_EQ(testingName, "Testing");
}

////////////////////////////////////////////////////////////////////////////////////////
///              Global namespace template class
////////////////////////////////////////////////////////////////////////////////////////

using GlobalNamespaceTemplatedClass = TemplateClass<int, bool, float>;

TEST_F(TypeNameComplexTest, GlobalNamespaceTemplatedClass3Arg)
{
    constexpr auto testingName = typeName<GlobalNamespaceTemplatedClass>();

    ASSERT_EQ(testingName, "TemplateClass<int32, bool, float>");
}

TEST_F(TypeNameComplexTest, GlobalNamespaceTemplatedClass3ArgQualified)
{
    constexpr auto testingName = typeNameQualified<GlobalNamespaceTemplatedClass>();

    ASSERT_EQ(testingName, "TemplateClass<int32, bool, float>");
}

// Ex1
using GlobalNamespaceTemplatedClassEx1 = TemplateClass<int64_t, uint32_t, int16_t>;

TEST_F(TypeNameComplexTest, GlobalNamespaceTemplatedClass3ArgEx1)
{
    constexpr auto testingName = typeName<GlobalNamespaceTemplatedClassEx1>();

    ASSERT_EQ(testingName, "TemplateClass<int64, uint32, int16>");
}

TEST_F(TypeNameComplexTest, GlobalNamespaceTemplatedClass3ArgQualifiedEx1)
{
    constexpr auto testingName = typeNameQualified<GlobalNamespaceTemplatedClassEx1>();

    ASSERT_EQ(testingName, "TemplateClass<int64, uint32, int16>");
}

// Ex2
using GlobalNamespaceTemplatedClassEx2 = TemplateClass<int8_t, uint64_t, uint16_t>;

TEST_F(TypeNameComplexTest, GlobalNamespaceTemplatedClass3ArgEx2)
{
    constexpr auto testingName = typeName<GlobalNamespaceTemplatedClassEx2>();

    ASSERT_EQ(testingName, "TemplateClass<int8, uint64, uint16>");
}

TEST_F(TypeNameComplexTest, GlobalNamespaceTemplatedClass3ArgQualifiedEx3)
{
    constexpr auto testingName = typeNameQualified<GlobalNamespaceTemplatedClassEx2>();

    ASSERT_EQ(testingName, "TemplateClass<int8, uint64, uint16>");
}

TEST_F(TypeNameComplexTest, GlobalTemplatedClassWithTemplateArgs)
{
    constexpr auto testingName = typeNameQualified<TemplatedGlobal<GlobalNamespaceTemplatedClassEx2>>();

    ASSERT_EQ(testingName, "TemplatedGlobal<TemplateClass<int8, uint64, uint16>>");
}

////////////////////////////////////////////////////////////////////////////////////////
///              Global namespace template struct
////////////////////////////////////////////////////////////////////////////////////////

using GlobalNamespaceTemplatedStruct = TemplateStruct<int, bool, float>;

TEST_F(TypeNameComplexTest, GlobalNamespaceTemplatedStruct3Arg)
{
    constexpr auto testingName = typeName<GlobalNamespaceTemplatedStruct>();

    ASSERT_EQ(testingName, "TemplateStruct<int32, bool, float>");
}

TEST_F(TypeNameComplexTest, GlobalNamespaceTemplatedStruct3ArgQualified)
{
    constexpr auto testingName = typeNameQualified<GlobalNamespaceTemplatedStruct>();

    ASSERT_EQ(testingName, "TemplateStruct<int32, bool, float>");
}

////////////////////////////////////////////////////////////////////////////////////////
///                 Namespaced struct
////////////////////////////////////////////////////////////////////////////////////////

using NamespacedStruct = ::Test::Foo::Bar::Record;

TEST_F(TypeNameComplexTest, NamespacedStruct)
{
    constexpr auto testingName = typeName<NamespacedStruct>();

    ASSERT_EQ(testingName, "Record");
}

TEST_F(TypeNameComplexTest, NamespacedStructQualified)
{
    constexpr auto testingName = typeNameQualified<NamespacedStruct>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::Record");
}

////////////////////////////////////////////////////////////////////////////////////////
///                 Namespaced templated struct
////////////////////////////////////////////////////////////////////////////////////////

using NamespacedTemplatedStruct3Arg = ::Test::Foo::Bar::StructRecord<int, bool, float>;
using NamespacedTemplatedStruct2Arg = ::Test::Foo::Bar::Struct<int, bool>;
using NamespacedTemplatedStruct1Arg = ::Test::Foo::Bar::RecordStruct<int>;

TEST_F(TypeNameComplexTest, NamespacedTemplatedStruct1Arg)
{
    constexpr auto testingName = typeName<NamespacedTemplatedStruct1Arg>();

    ASSERT_EQ(testingName, "RecordStruct<int32>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedStruct1ArgQualified)
{
    constexpr auto testingName = typeNameQualified<NamespacedTemplatedStruct1Arg>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::RecordStruct<int32>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedStruct2Arg)
{
    constexpr auto testingName = typeName<NamespacedTemplatedStruct2Arg>();

    ASSERT_EQ(testingName, "Struct<int32, bool>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedStruct2ArgQualified)
{
    constexpr auto testingName = typeNameQualified<NamespacedTemplatedStruct2Arg>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::Struct<int32, bool>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedStruct3Arg)
{
    constexpr auto testingName = typeName<NamespacedTemplatedStruct3Arg>();

    ASSERT_EQ(testingName, "StructRecord<int32, bool, float>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedStruct3ArgQualified)
{
    constexpr auto testingName = typeNameQualified<NamespacedTemplatedStruct3Arg>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::StructRecord<int32, bool, float>");
}

////////////////////////////////////////////////////////////////////////////////////////
///                 Namespaced class
////////////////////////////////////////////////////////////////////////////////////////

using NamespacedClass = Test::Foo::Bar::Testing;

TEST_F(TypeNameComplexTest, NamespacedClass)
{
    constexpr auto testingName = typeName<NamespacedClass>();

    ASSERT_EQ(testingName, "Testing");
}

TEST_F(TypeNameComplexTest, NamespacedClassQualified)
{
    constexpr auto testingName = typeNameQualified<NamespacedClass>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::Testing");
}

////////////////////////////////////////////////////////////////////////////////////////
///                 Namespaced templated class
////////////////////////////////////////////////////////////////////////////////////////

using NamespacedTemplatedClass3Arg = Test::Foo::Bar::BarFoo<int, bool, float>;
using NamespacedTemplatedClass2Arg = Test::Foo::Bar::FooBar<int, bool>;
using NamespacedTemplatedClass1Arg = Test::Foo::Bar::Foo<int>;

TEST_F(TypeNameComplexTest, NamespacedTemplatedClass3Arg)
{
    constexpr auto testingName = typeName<NamespacedTemplatedClass3Arg>();

    ASSERT_EQ(testingName, "BarFoo<int32, bool, float>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedClass3ArgQualified)
{
    constexpr auto testingName = typeNameQualified<NamespacedTemplatedClass3Arg>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::BarFoo<int32, bool, float>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedClass2Arg)
{
    constexpr auto testingName = typeName<NamespacedTemplatedClass2Arg>();

    ASSERT_EQ(testingName, "FooBar<int32, bool>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedClass2ArgQualified)
{
    constexpr auto testingName = typeNameQualified<NamespacedTemplatedClass2Arg>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::FooBar<int32, bool>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedClass1Arg)
{
    constexpr auto testingName = typeName<NamespacedTemplatedClass1Arg>();

    ASSERT_EQ(testingName, "Foo<int32>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedClass1ArgQualified)
{
    constexpr auto testingName = typeNameQualified<NamespacedTemplatedClass1Arg>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::Foo<int32>");
}


TEST_F(TypeNameComplexTest, NamespacedTemplatedWithNamespacedArgument)
{
    using namespace ::Test::Foo;
    constexpr auto testingName = typeName<Bar::Foo<Bar::Testing>>();

    ASSERT_EQ(testingName, "Foo<Testing>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedWithNamespacedArgumentQualified)
{
    using namespace ::Test::Foo;
    constexpr auto testingName = typeNameQualified<Bar::Foo<Bar::Testing>>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::Foo<Test::Foo::Bar::Testing>");
}


////

TEST_F(TypeNameComplexTest, NamespacedTemplatedWithGlobalTemplate)
{
    using namespace ::Test::Foo;
    constexpr auto testingName = typeName<Bar::Foo<TemplatedGlobal<int32_t>>>();

    ASSERT_EQ(testingName, "Foo<TemplatedGlobal<int32>>");
}

TEST_F(TypeNameComplexTest, NamespacedTemplatedWithGlobalTemplateQualified)
{
    using namespace ::Test::Foo;
    constexpr auto testingName = typeNameQualified<Bar::Foo<TemplatedGlobal<int32_t>>>();

    ASSERT_EQ(testingName, "Test::Foo::Bar::Foo<TemplatedGlobal<int32>>");
}
