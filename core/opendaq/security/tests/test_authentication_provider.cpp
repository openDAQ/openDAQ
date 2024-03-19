#include <gtest/gtest.h>
#include <opendaq/authentication_provider_factory.h>
#include <opendaq/security_exceptions.h>
#include <opendaq/user_factory.h>

using namespace daq;

using AuthenticationProviderTest = testing::Test;

TEST_F(AuthenticationProviderTest, Default)
{
    auto provider = AuthenticationProvider();

    ASSERT_TRUE(provider.assigned());
    ASSERT_THROW(provider.authenticate("", ""), AuthenticationFailedException);
    ASSERT_THROW(provider.authenticate("user", "user"), AuthenticationFailedException);
}

TEST_F(AuthenticationProviderTest, Static)
{
    auto user1 = User("user", "pass");
    auto user2 = User("admin", "admin");
    auto users = List<IUser>(user1, user2);

    auto provider = StaticAuthenticationProvider(users);
    ASSERT_TRUE(provider.assigned());
    ASSERT_THROW(provider.authenticate("manager", ""), AuthenticationFailedException);
    ASSERT_THROW(provider.authenticate("user", ""), AuthenticationFailedException);

    auto authenitcatedUser1 = provider.authenticate("user", "pass");
    ASSERT_EQ(authenitcatedUser1.getUsername(), "user");

    auto authenitcatedUser2 = provider.authenticate("admin", "admin");
    ASSERT_EQ(authenitcatedUser2.getUsername(), "admin");
}

TEST_F(AuthenticationProviderTest, JsonMissingFile)
{
    ASSERT_THROW(JsonFileAuthenticationProvider("missingFile.json"), NotFoundException);
}

TEST_F(AuthenticationProviderTest, InvalidJson)
{
    const auto invalidJson = "[{\"__type\":\" User \",";
    ASSERT_THROW(JsonStringAuthenticationProvider(invalidJson), DeserializeException);
}

TEST_F(AuthenticationProviderTest, EmptyJson)
{
    auto provider = JsonStringAuthenticationProvider("   ");
    ASSERT_TRUE(provider.assigned());
}

TEST_F(AuthenticationProviderTest, Json)
{
    auto user1 = User("user", "pass");
    auto user2 = User("admin", "admin");
    auto users = List<IUser>(user1, user2);

    auto serializer = JsonSerializer();
    users.serialize(serializer);
    const auto json = serializer.getOutput();

    auto provider = JsonStringAuthenticationProvider(json);
    ASSERT_TRUE(provider.assigned());

    ASSERT_THROW(provider.authenticate("manager", ""), AuthenticationFailedException);
    ASSERT_THROW(provider.authenticate("user", ""), AuthenticationFailedException);

    auto authenitcatedUser1 = provider.authenticate("user", "pass");
    ASSERT_EQ(authenitcatedUser1.getUsername(), "user");

    auto authenitcatedUser2 = provider.authenticate("admin", "admin");
    ASSERT_EQ(authenitcatedUser2.getUsername(), "admin");
}

