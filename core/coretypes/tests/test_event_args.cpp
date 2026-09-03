#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using namespace daq;

// unity-safe namespace: keeps this file's test types file-local
namespace test_event_args
{

using EventArgsTest = testing::Test;


TEST_F(EventArgsTest, Test)
{
    IEventArgs* args;

    //createEventArgs(&args, 5, String_Create("test"));

    createEventArgs(&args, 5, String("test"));

    args->releaseRef();
}

TEST_F(EventArgsTest, TestPtr)
{
    auto eventArgs = EventArgs(0, "test");
}

TEST_F(EventArgsTest, Inspectable)
{
    auto obj = EventArgs(0, "test");

    auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
    ASSERT_EQ(ids[0], IEventArgs::Id);
}

TEST_F(EventArgsTest, ImplementationName)
{
    auto obj = EventArgs(0, "test");

    std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
    auto prefix = className.find("daq::EventArgsImpl");

    ASSERT_EQ(prefix, 0u);
}

static constexpr auto EVENT_ARGS_INTERFACE_ID = FromTemplatedTypeName("IEventArgs", "daq");

TEST_F(EventArgsTest, InterfaceId)
{
    ASSERT_EQ(EVENT_ARGS_INTERFACE_ID, IEventArgs::Id);
}

TEST_F(EventArgsTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IEventArgs>(), "{81D0979C-1FA7-51F8-80FB-44216A6F8D33}");
}
}
// namespace test_event_args
