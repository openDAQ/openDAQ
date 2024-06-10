#include <gtest/gtest.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/exceptions.h>

using namespace daq;

using AuthenticationProviderTest = testing::Test;

TEST_F(AuthenticationProviderTest, Default)
{
    auto provider = AuthenticationProvider();

    ASSERT_TRUE(provider.assigned());
    ASSERT_THROW(provider.authenticate("", ""), AuthenticationFailedException);
    ASSERT_THROW(provider.authenticate("user", "user"), AuthenticationFailedException);

    ASSERT_TRUE(provider.isAnonymousAllowed());
}

TEST_F(AuthenticationProviderTest, NoAnonymous)
{
    auto provider = AuthenticationProvider(false);

    ASSERT_TRUE(provider.assigned());
    ASSERT_FALSE(provider.isAnonymousAllowed());
}

TEST_F(AuthenticationProviderTest, Static)
{
    auto user1 = User("user", "pass");
    auto user2 = User("admin", "admin");
    auto users = List<IUser>(user1, user2);

    auto provider = StaticAuthenticationProvider(false, users);
    ASSERT_TRUE(provider.assigned());
    ASSERT_THROW(provider.authenticate("manager", ""), AuthenticationFailedException);
    ASSERT_THROW(provider.authenticate("user", ""), AuthenticationFailedException);

    auto authenitcatedUser1 = provider.authenticate("user", "pass");
    ASSERT_EQ(authenitcatedUser1.getUsername(), "user");

    auto authenitcatedUser2 = provider.authenticate("admin", "admin");
    ASSERT_EQ(authenitcatedUser2.getUsername(), "admin");

    ASSERT_FALSE(provider.isAnonymousAllowed());
}

TEST_F(AuthenticationProviderTest, Json)
{
    const std::string json = R"(
        {
            "allowAnonymous": true,
            "users" : [
                {"username" : "user", "passwordHash" : "pass", "groups" : [ "user", "guest" ]},
                {"username" : "admin", "passwordHash" : "admin", "groups" : []},
                {"username" : "jure", "passwordHash" : "jure"}
            ]
        }
    )";

    auto provider = JsonStringAuthenticationProvider(json);
    ASSERT_TRUE(provider.assigned());
    ASSERT_TRUE(provider.isAnonymousAllowed());

    ASSERT_THROW(provider.authenticate("manager", ""), AuthenticationFailedException);
    ASSERT_THROW(provider.authenticate("user", ""), AuthenticationFailedException);

    auto authenitcatedUser1 = provider.authenticate("user", "pass");
    ASSERT_EQ(authenitcatedUser1.getUsername(), "user");

    auto authenitcatedUser2 = provider.authenticate("admin", "admin");
    ASSERT_EQ(authenitcatedUser2.getUsername(), "admin");
}

TEST_F(AuthenticationProviderTest, JsonNoUsers)
{
    const std::string json = R"({"allowAnonymous": false})";

    auto provider = JsonStringAuthenticationProvider(json);
    ASSERT_TRUE(provider.assigned());
    ASSERT_FALSE(provider.isAnonymousAllowed());
}


TEST_F(AuthenticationProviderTest, JsonMissingFile)
{
    ASSERT_THROW(JsonFileAuthenticationProvider("missingFile.json"), NotFoundException);
}

TEST_F(AuthenticationProviderTest, InvalidJson)
{
    const auto invalidJson = "[{\"__type\":\" User \",";
    ASSERT_THROW(JsonStringAuthenticationProvider(invalidJson), ParseFailedException);
}

TEST_F(AuthenticationProviderTest, EmptyJson)
{
    ASSERT_THROW(JsonStringAuthenticationProvider("   "), ParseFailedException);
}

