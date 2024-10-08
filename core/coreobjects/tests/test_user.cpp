#include <gtest/gtest.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/user_internal_ptr.h>

using namespace daq;

using UserTest = testing::Test;

TEST_F(UserTest, Create)
{
    auto user = User("janm", "pass", List<IString>("admin", "user"));
    ASSERT_TRUE(user.assigned());
}

TEST_F(UserTest, EmptyGroups)
{
    auto user = User("janm", "psswordHash");

    ASSERT_TRUE(user.getGroups().assigned());
    ASSERT_EQ(user.getGroups().getCount(), 1u);
    ASSERT_EQ(user.getGroups().getItemAt(0), "everyone");
}


TEST_F(UserTest, SanitazeGroups)
{
    const auto groups = List<IString>("user", "user", "admin");
    const auto groupsExpected = List<IString>("admin", "everyone", "user");

    auto user = User("janm", "pass", groups);

    ASSERT_TRUE(user.getGroups().assigned());
    ASSERT_EQ(user.getGroups(), groupsExpected);
}

TEST_F(UserTest, Getters)
{
    const auto groups = List<IString>("admin", "user");
    const auto groupsExpected = List<IString>("admin", "everyone", "user");

    auto user = User("janm", "pass", groups);

    ASSERT_EQ(user.getUsername(), "janm");
    ASSERT_EQ(user.getGroups(), groupsExpected);

    auto userInternal = user.asPtr<IUserInternal>();
    ASSERT_EQ(userInternal.getPasswordHash(), "pass");
}

TEST_F(UserTest, Equals)
{
    auto user1 = User("user", "psswordHash", List<IString>("admin", "user"));
    auto user2 = User("user", "psswordHash", List<IString>("admin", "user"));
    auto user3 = User("user3", "psswordHash3");

    ASSERT_EQ(user1.getUsername(), user2.getUsername());
    ASSERT_EQ(user1.getGroups(), user2.getGroups());
    ASSERT_EQ(user1.asPtr<IUserInternal>().getPasswordHash(), user2.asPtr<IUserInternal>().getPasswordHash());

    ASSERT_TRUE(BaseObjectPtr::Equals(user1, user1));
    ASSERT_TRUE(BaseObjectPtr::Equals(user1, user2));
    ASSERT_FALSE(BaseObjectPtr::Equals(user1, user3));
}

TEST_F(UserTest, Serialize)
{
    auto user1 = User("janm", "psswordHash", List<IString>("admin", "user"));

    auto serializer = JsonSerializer();
    user1.serialize(serializer);
    const auto json = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    UserPtr user2 = deserializer.deserialize(json);

    ASSERT_TRUE(BaseObjectPtr::Equals(user1, user1));
}

TEST_F(UserTest, SerializeEmptyGroups)
{
    auto user1 = User("janm", "psswordHash");

    auto serializer = JsonSerializer();
    user1.serialize(serializer);
    const auto json = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    UserPtr user2 = deserializer.deserialize(json);

    ASSERT_TRUE(BaseObjectPtr::Equals(user1, user1));
}

TEST_F(UserTest, IsAnonymous)
{
    auto userJan = User("jan", "psswordHash");
    auto userEmpty = User("", "psswordHash");
    auto userAnonymous = User("", "");

    ASSERT_FALSE(userJan.asPtr<IUserInternal>().isAnonymous());
    ASSERT_FALSE(userEmpty.asPtr<IUserInternal>().isAnonymous());
    ASSERT_TRUE(userAnonymous.asPtr<IUserInternal>().isAnonymous());
}
