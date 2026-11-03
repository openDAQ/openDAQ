#include <gtest/gtest.h>
#include <opendaq/login_parameters_factory.h>
#include <opendaq/authentication_method_factory.h>

using LoginParametersTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(LoginParametersTest, BuilderSettersGetters)
{
    const auto method = StandardUserNamePasswordAuthenticationMethod();
    const auto credentials = Dict<IString, IString>({{"UserName", "john"}, {"Password", "secret"}});

    auto builder = LoginParametersBuilder();
    builder.setAuthenticationMethod(method);
    builder.setCredentials(credentials);

    ASSERT_EQ(builder.getAuthenticationMethod().getId(), StandardUserNamePasswordId);
    ASSERT_EQ(builder.getCredentials(), credentials);
}

TEST_F(LoginParametersTest, BuildThrowsWhenAuthenticationMethodUnassigned)
{
    auto builder = LoginParametersBuilder();
    builder.setCredentials(Dict<IString, IString>());

    ASSERT_THROW(builder.build(), InvalidParameterException);
}

TEST_F(LoginParametersTest, BuildSucceedsForAnonymousMethod)
{
    const auto emptyCredentials = Dict<IString, IString>();
    {
        auto builder = LoginParametersBuilder();
        builder.setAuthenticationMethod(StandardAnonymousAuthenticationMethod());
        builder.setCredentials(emptyCredentials);

        const auto parameters = builder.build();

        ASSERT_EQ(parameters.getAuthenticationMethod().getId(), StandardAnonymousId);
        ASSERT_EQ(parameters.getCredentials(), emptyCredentials);
    }
    {
        // Credentials default to empty whenever never set on the builder.
        auto builder = LoginParametersBuilder();
        builder.setAuthenticationMethod(StandardAnonymousAuthenticationMethod());

        const auto parameters = builder.build();

        ASSERT_EQ(parameters.getAuthenticationMethod().getId(), StandardAnonymousId);
        ASSERT_EQ(parameters.getCredentials(), emptyCredentials);
    }
}

TEST_F(LoginParametersTest, BuildThrowsWhenCredentialsDoNotSatisfyMethod)
{
    {
        // Credentials default to empty when never set, but an empty credential doesn't satisfy a method with
        // mandatory fields, so build() fails.
        auto builder = LoginParametersBuilder();
        builder.setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod());

        ASSERT_THROW(builder.build(), InvalidParameterException);
    }
    {
        auto builder = LoginParametersBuilder();
        builder.setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod());
        builder.setCredentials(Dict<IString, IString>({{"UserName", "john"}}));

        ASSERT_THROW(builder.build(), InvalidParameterException);
    }
}

TEST_F(LoginParametersTest, ParametersCarryBuilderValues)
{
    const auto method = StandardPinAuthenticationMethod();
    const auto credentials = Dict<IString, IString>({{"Pin", "1234"}});

    auto builder = LoginParametersBuilder();
    builder.setAuthenticationMethod(method);
    builder.setCredentials(credentials);

    const auto parameters = builder.build();

    ASSERT_EQ(parameters.getAuthenticationMethod().getId(), StandardPinId);
    ASSERT_EQ(parameters.getCredentials(), credentials);
}

END_NAMESPACE_OPENDAQ
