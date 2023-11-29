#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/coretype.h>
#include <coretypes/ctutils.h>

using IntfIdTest = testing::Test;
using namespace daq;

struct IBaseObjectTest : IUnknown
{
    DEFINE_INTFID("IBaseObject")
};

template <class T>
struct ITemplatedIntf : IUnknown
{
    DEFINE_INTFID("ITemplatedInterface", T)
};

template <class T, class U>
struct ITemplated2Intf : IUnknown
{
    DEFINE_INTFID("ITemplated2Interface", T, U)
};

TEST_F(IntfIdTest, ActualBaseObjectGuid)
{
    constexpr IntfID baseObj = IntfID::FromTypeName("IBaseObject.daq");
    bool eq = baseObj == IBaseObject::Id;

    ASSERT_TRUE(eq);
}

TEST_F(IntfIdTest, TestSize)
{
    constexpr IntfID baseObj = IntfID::FromTypeName("IBaseObject.daq");
    static_assert(sizeof(baseObj) == 16);
}

TEST_F(IntfIdTest, ActualBaseObjectGuidExplicit)
{
    constexpr IntfID baseObj = { 0xe8f364f8, 0xe940, 0x572d, { { 0xbb, 0x89, 0x8a, 0x7d, 0x2a, 0xe1, 0xdd, 0xe7 } } };
    bool eq = baseObj == IBaseObject::Id;

    ASSERT_TRUE(eq);
}

TEST_F(IntfIdTest, BaseObjectGuid)
{
    constexpr IntfID baseObj = IntfID::FromTypeName("IBaseObject.daq");
    bool eq = baseObj == IBaseObjectTest::Id;

    ASSERT_TRUE(eq);
}

TEST_F(IntfIdTest, TemplatedTest)
{
    constexpr IntfID id = IntfID::FromTypeName("ITemplatedInterface<int32>.daq");

#if !(defined(__GNUC__) && defined(NDEBUG))
    /* does not compile with GCC on release mode: http://eel.is/c++draft/temp#res-8
     *
     * The program is ill-formed, ***no diagnostic required***, if:
     *   - no valid specialization can be generated for a template or a sub-statement of a constexpr if statement ([stmt.if])
     *     within a template and the template is not instantiated
     */

    // Compile time check
    static_assert(IntfID::Compare(id, ITemplatedIntf<int32_t>::Id));
#endif

    // runtime time check
    ASSERT_EQ(id, ITemplatedIntf<int32_t>::Id);
}

TEST_F(IntfIdTest, Templated2Test)
{
    constexpr IntfID id1 = IntfID::FromTypeName("ITemplated2Interface<int32,uint16>.daq");
    constexpr IntfID id2 = IntfID::FromTypeName("ITemplated2Interface<float,double>.daq");

#if !(defined(__GNUC__) && defined(NDEBUG))

    /* does not compile with GCC on release mode: http://eel.is/c++draft/temp#res-8
     *
     * The program is ill-formed, ***no diagnostic required***, if:
     *   - no valid specialization can be generated for a template or a sub-statement of a constexpr if statement ([stmt.if]) within a template and the template is not instantiated
     */

    // Compile time check
    static_assert(IntfID::Compare(id1, ITemplated2Intf<int32_t, uint16_t>::Id), "Text and interface id must match");
    static_assert(IntfID::Compare(id2, ITemplated2Intf<float, double>::Id), "Text and interface id must match");
#endif

    // runtime time check
    ASSERT_EQ(id1, (ITemplated2Intf<int32_t, uint16_t>::Id));
    ASSERT_EQ(id2, (ITemplated2Intf<float, double>::Id));
}

TEST_F(IntfIdTest, CoreTypeId)
{
    constexpr auto id = OPENDAQ_INTFID("ICoreType");

    ASSERT_EQ(id, ICoreType::Id);
}

TEST_F(IntfIdTest, CoreTypeIdExplicit)
{
    constexpr IntfID id = { 0x72e0d318, 0x84dd, 0x589f, { { 0xaa, 0x58, 0xd5, 0x70, 0xb8, 0x1c, 0xd7, 0x7d } } };

    ASSERT_EQ(id, ICoreType::Id);
}

TEST_F(IntfIdTest, Hash)
{
    std::hash<daq::IntfID> hash{};
    ASSERT_EQ(hash(IBaseObjectTest::Id), hash(IBaseObjectTest::Id));
    ASSERT_NE(hash(IBaseObjectTest::Id), hash(IBaseObjectTest::Id));
}
