#include <gtest/gtest.h>
#include <coretypes/coretypes.h>

using EventArgsTest = testing::Test;

using namespace daq;

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

static constexpr auto INTERFACE_ID = FromTemplatedTypeName("IEventArgs", "daq");

TEST_F(EventArgsTest, InterfaceId)
{
    ASSERT_EQ(INTERFACE_ID, IEventArgs::Id);
}

TEST_F(EventArgsTest, InterfaceIdString)
{
    ASSERT_EQ(daqInterfaceIdString<IEventArgs>(), "{81D0979C-1FA7-51F8-80FB-44216A6F8D33}");
}
