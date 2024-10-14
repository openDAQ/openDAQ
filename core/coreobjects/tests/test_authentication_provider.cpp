#include <gtest/gtest.h>
#include <coreobjects/authentication_provider_factory.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/exceptions.h>
#include <coreobjects/user_internal_ptr.h>

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

TEST_F(AuthenticationProviderTest, AuthenticatePlain)
{
    auto users = List<IUser>();
    users.pushBack(User("user", "user123"));
    users.pushBack(User("admin", "admin123"));
    users.pushBack(User("guest", "$2a$Password"));

    auto provider = StaticAuthenticationProvider(false, users);

    ASSERT_THROW(provider.authenticate("user", "wrongPass"), AuthenticationFailedException);
    ASSERT_EQ(provider.authenticate("user", "user123").getUsername(), "user");

    ASSERT_THROW(provider.authenticate("admin", "wrongPass"), AuthenticationFailedException);
    ASSERT_EQ(provider.authenticate("admin", "admin123").getUsername(), "admin");

    ASSERT_THROW(provider.authenticate("guest", "wrongPass"), AuthenticationFailedException);
    ASSERT_EQ(provider.authenticate("guest", "$2a$Password").getUsername(), "guest");
}

TEST_F(AuthenticationProviderTest, AuthenticateBcrypt)
{
    auto users = List<IUser>();
    users.pushBack(User("user", "$2a$12$9gJbsmTbR3Vh2C.6l83roe/fWVunnXa3/1AbtEYXuh20OZBkXD2Hy")); // user123
    users.pushBack(User("admin", "$2a$12$vEipziDschmoMw2iVaoUbeDX3p1u3W8NQn.wgu0KkJn/C.tvIPEpG")); // admin123

    auto provider = StaticAuthenticationProvider(false, users);

    ASSERT_THROW(provider.authenticate("user", "wrongPass"), AuthenticationFailedException);
    ASSERT_THROW(provider.authenticate("user", "$2a$12$9gJbsmTbR3Vh2C.6l83roe/fWVunnXa3/1AbtEYXuh20OZBkXD2Hy"), AuthenticationFailedException);
    ASSERT_EQ(provider.authenticate("user", "user123").getUsername(), "user");

    ASSERT_THROW(provider.authenticate("admin", "wrongPass"), AuthenticationFailedException);
    ASSERT_THROW(provider.authenticate("admin", "$2a$12$vEipziDschmoMw2iVaoUbeDX3p1u3W8NQn.wgu0KkJn/C.tvIPEpG"), AuthenticationFailedException);
    ASSERT_EQ(provider.authenticate("admin", "admin123").getUsername(), "admin");
}

TEST_F(AuthenticationProviderTest, AuthanticateAnonymous)
{
    auto provider = AuthenticationProvider(true);
    auto user1 = provider.authenticateAnonymous();
    auto user2 = provider.authenticateAnonymous();

    ASSERT_EQ(user1, user2);
    ASSERT_EQ(user1.getUsername(), "");
    ASSERT_EQ(user1.asPtr<IUserInternal>().getPasswordHash(), "");
    ASSERT_EQ(user1.getGroups().getCount(), 1u);
    ASSERT_EQ(user1.getGroups()[0], "everyone");
}

TEST_F(AuthenticationProviderTest, AuthanticateAnonymousDisabled)
{
    auto provider = AuthenticationProvider(false);
    ASSERT_THROW(provider.authenticateAnonymous(), AuthenticationFailedException);
}

