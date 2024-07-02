#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/cloneable.h>
#include <coretypes/comparable.h>
#include <coretypes/convertible.h>
#include <coretypes/coretype.h>
#include <coretypes/ctutils.h>
#include <coretypes/dict_element_type.h>
#include <coretypes/event_handler.h>
#include <coretypes/freezable.h>
#include <coretypes/inspectable.h>
#include <coretypes/list_element_type.h>
#include <coretypes/number.h>
#include <coretypes/serializable.h>
#include <coretypes/updatable.h>
#include <coretypes/version_info.h>

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
    constexpr IntfID baseObj = { 0x9c911f6d, 0x1664, 0x5aa2, { { 0x97, 0xbd, 0x90, 0xfe, 0x31, 0x43, 0xe8, 0x81 } } };
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
    constexpr IntfID id = IntfID::FromTypeName("ITemplatedInterface<Int32>.daq");

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
    constexpr IntfID id1 = IntfID::FromTypeName("ITemplated2Interface<Int32,UInt16>.daq");
    constexpr IntfID id2 = IntfID::FromTypeName("ITemplated2Interface<Float,Double>.daq");

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

TEST_F(IntfIdTest, CoreTypeIdMacro)
{
    constexpr auto id = OPENDAQ_INTFID("ICoreType");

    ASSERT_EQ(id, ICoreType::Id);
}

TEST_F(IntfIdTest, CoreTypeIdExplicit)
{
    constexpr IntfID id = { 0x562d045, 0xc94e, 0x5e6d, { { 0x83, 0x60, 0x2c, 0xfc, 0x9d, 0xb7, 0x6a, 0x4 } } };
    ASSERT_EQ(id, ICoreType::Id);
}

static constexpr auto CORE_TYPE_ID = FromTemplatedTypeName("ICoreType", "daq");

TEST_F(IntfIdTest, CoreTypeId)
{
    ASSERT_EQ(CORE_TYPE_ID, ICoreType::Id);
}

TEST_F(IntfIdTest, CoreTypeIdString)
{
    ASSERT_EQ(daqInterfaceIdString<ICoreType>(), "{0562D045-C94E-5E6D-8360-2CFC9DB76A04}");
}

TEST_F(IntfIdTest, Hash)
{
    std::hash<daq::IntfID> hash{};
    ASSERT_EQ(hash(IBaseObjectTest::Id), hash(IBaseObjectTest::Id));
}

static constexpr auto COMPARABLE_ID = FromTemplatedTypeName("IComparable", "daq");

TEST_F(IntfIdTest, ComparableId)
{
    ASSERT_EQ(COMPARABLE_ID, IComparable::Id);
}

TEST_F(IntfIdTest, ComparableIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IComparable>(), "{70F80B82-40DD-5028-9DEE-915958A30747}");
}

static constexpr auto CONVERTIBLE_ID = FromTemplatedTypeName("IConvertible", "daq");

TEST_F(IntfIdTest, ConvertibleId)
{
    ASSERT_EQ(CONVERTIBLE_ID, IConvertible::Id);
}

TEST_F(IntfIdTest, ConvertibleIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IConvertible>(), "{D984FD0F-7980-5E7B-8EC1-75C507F302FE}");
}

static constexpr auto DICT_ELEMENT_TYPE_ID = FromTemplatedTypeName("IDictElementType", "daq");

TEST_F(IntfIdTest, DictElementTypeId)
{
    ASSERT_EQ(DICT_ELEMENT_TYPE_ID, IDictElementType::Id);
}

TEST_F(IntfIdTest, DictElementTypeIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IDictElementType>(), "{AD524EBE-B272-5657-B7A3-1B05F1B93EBC}");
}

static constexpr auto LIST_ELEMENT_TYPE_ID = FromTemplatedTypeName("IListElementType", "daq");

TEST_F(IntfIdTest, ListElementTypeId)
{
    ASSERT_EQ(LIST_ELEMENT_TYPE_ID, IListElementType::Id);
}

TEST_F(IntfIdTest, ListElementTypeIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IListElementType>(), "{F63FCBD4-FC76-54FF-81EC-F85307646BC7}");
}

static constexpr auto EVENT_HANDLER_ID = FromTemplatedTypeName("IEventHandler", "daq");

TEST_F(IntfIdTest, EventHandlerId)
{
    ASSERT_EQ(EVENT_HANDLER_ID, IEventHandler::Id);
}

TEST_F(IntfIdTest, EventHandlerIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IEventHandler>(), "{8173CD51-2DE8-5DF3-A729-8A2728637DAD}");
}

static constexpr auto FREEZABLE_ID = FromTemplatedTypeName("IFreezable", "daq");

TEST_F(IntfIdTest, FreezableId)
{
    ASSERT_EQ(FREEZABLE_ID, IFreezable::Id);
}

TEST_F(IntfIdTest, FreezableString)
{
    ASSERT_EQ(daqInterfaceIdString<IFreezable>(), "{06F0D04D-3CA7-5E0F-A6CB-2459608C6519}");
}

static constexpr auto INSPECTABLE_ID = FromTemplatedTypeName("IInspectable", "daq");

TEST_F(IntfIdTest, InspectableId)
{
    ASSERT_EQ(INSPECTABLE_ID, IInspectable::Id);
}

TEST_F(IntfIdTest, InspectableString)
{
    ASSERT_EQ(daqInterfaceIdString<IInspectable>(), "{9869DF21-C7B3-5E0E-8E4B-66DB6A7265A8}");
}

static constexpr auto NUMBER_ID = FromTemplatedTypeName("INumber", "daq");

TEST_F(IntfIdTest, NumberId)
{
    ASSERT_EQ(NUMBER_ID, INumber::Id);
}

TEST_F(IntfIdTest, NumberString)
{
    ASSERT_EQ(daqInterfaceIdString<INumber>(), "{52711B8D-DF25-59B0-AF86-1015C7B54603}");
}

static constexpr auto UPDATABLE_ID = FromTemplatedTypeName("IUpdatable", "daq");

TEST_F(IntfIdTest, UpdatableId)
{
    ASSERT_EQ(UPDATABLE_ID, IUpdatable::Id);
}

TEST_F(IntfIdTest, UpdatableString)
{
    ASSERT_EQ(daqInterfaceIdString<IUpdatable>(), "{94BF8B0E-2868-51A2-8773-CBB98A4DD1BE}");
}

static constexpr auto VERSION_INFO_ID = FromTemplatedTypeName("IVersionInfo", "daq");

TEST_F(IntfIdTest, VersionInfoId)
{
    ASSERT_EQ(VERSION_INFO_ID, IVersionInfo::Id);
}

TEST_F(IntfIdTest, VersionInfoString)
{
    ASSERT_EQ(daqInterfaceIdString<IVersionInfo>(), "{5951D4D2-35EB-513C-B67D-89DABC6BE3BF}");
}
