#include <gtest/gtest.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/authentication_method_factory.h>

using AuthenticationConfigTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

static AuthenticationConfigPtr MakeAuthenticationConfig(const DictPtr<IString, IAuthenticationMethod>& methods)
{
    return AuthenticationConfigPtr(AuthenticationConfig_Create(methods));
}

static DictPtr<IString, IAuthenticationMethod> AnonymousAndUserNamePasswordMethods()
{
    return Dict<IString, IAuthenticationMethod>({{StandardAnonymousId, StandardAnonymousAuthenticationMethod()},
                                                 {StandardUserNamePasswordId, StandardUserNamePasswordAuthenticationMethod()}});
}

TEST_F(AuthenticationConfigTest, DefaultsToFirstEntryToBeSelected)
{
    const auto methods = AnonymousAndUserNamePasswordMethods();
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(methods);

    ASSERT_EQ(config.getSelectedAuthenticationMethod().getId(), StandardAnonymousId);
}

TEST_F(AuthenticationConfigTest, ThrowsOnEmptyOrUnassignedMethods)
{
    ASSERT_THROW(MakeAuthenticationConfig(nullptr), InvalidParameterException);
    ASSERT_THROW(MakeAuthenticationConfig(Dict<IString, IAuthenticationMethod>()), InvalidParameterException);
}

TEST_F(AuthenticationConfigTest, SetAuthenticationMethodSwitchesSelection)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());

    config.setAuthenticationMethod(StandardUserNamePasswordId);

    ASSERT_EQ(config.getSelectedAuthenticationMethod().getId(), StandardUserNamePasswordId);
}

TEST_F(AuthenticationConfigTest, SetAuthenticationMethodThrowsOnUnknownId)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());

    ASSERT_THROW(config.setAuthenticationMethod("NoSuchMethod"), NotFoundException);
}

TEST_F(AuthenticationConfigTest, SettingAuthenticationMethodResetsSuppliedCredential)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());
    config.setAuthenticationMethod(StandardUserNamePasswordId);

    const auto credentials = Dict<IString, IString>({{"UserName", "john"}, {"Password", "secret"}});
    config.setSuppliedCredential(credentials);
    ASSERT_EQ(config.getSuppliedCredential(), credentials);

    config.setAuthenticationMethod(StandardAnonymousId);

    const auto emptyCredentials = Dict<IString, IString>();
    ASSERT_EQ(config.getSuppliedCredential(), emptyCredentials);
}

TEST_F(AuthenticationConfigTest, GetAuthenticationMethodsReturnsSupportedSet)
{
    const auto methods = AnonymousAndUserNamePasswordMethods();
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(methods);

    ASSERT_EQ(config.getAuthenticationMethods(), methods);
}

TEST_F(AuthenticationConfigTest, SuppliedCredentialDefaultsEmpty)
{
    const AuthenticationConfigPtr config =
        MakeAuthenticationConfig(Dict<IString, IAuthenticationMethod>({{StandardUserNamePasswordId, StandardUserNamePasswordAuthenticationMethod()}}));

    const auto credential = config.getSuppliedCredential();
    const auto emptyCredentials = Dict<IString, IString>();
    ASSERT_TRUE(credential.assigned());
    ASSERT_EQ(credential, emptyCredentials);
}

TEST_F(AuthenticationConfigTest, SetSuppliedCredentialAcceptedWhenSatisfiesMethod)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());
    config.setAuthenticationMethod(StandardUserNamePasswordId);

    const auto credentials = Dict<IString, IString>({{"UserName", "john"}, {"Password", "secret"}});
    config.setSuppliedCredential(credentials);

    ASSERT_EQ(config.getSuppliedCredential(), credentials);
}

TEST_F(AuthenticationConfigTest, SetSuppliedCredentialThrowsWhenMissingRequiredField)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());
    config.setAuthenticationMethod(StandardUserNamePasswordId);

    ASSERT_THROW(config.setSuppliedCredential(Dict<IString, IString>({{"UserName", "john"}})), InvalidParameterException);
}

TEST_F(AuthenticationConfigTest, SetSuppliedCredentialEmptyClearsManually)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());
    config.setAuthenticationMethod(StandardUserNamePasswordId);

    auto credentials = Dict<IString, IString>({{"UserName", "john"}, {"Password", "secret"}});
    ASSERT_NO_THROW(config.setSuppliedCredential(credentials));
    ASSERT_EQ(config.getSuppliedCredential(), credentials);

    credentials = Dict<IString, IString>();
    ASSERT_NO_THROW(config.setSuppliedCredential(credentials));
    ASSERT_EQ(config.getSuppliedCredential(), credentials);
}

TEST_F(AuthenticationConfigTest, SetSuppliedCredentialThrowsNotSupportedForAnon)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());

    ASSERT_THROW(config.setSuppliedCredential(Dict<IString, IString>({{"Whatever", "value"}})), NotSupportedException);
}

TEST_F(AuthenticationConfigTest, GenericPropertyObjectAccess)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());

    const StringPtr selectedValue = config.getPropertySelectionValue("AuthenticationMethod");
    ASSERT_EQ(selectedValue, StandardAnonymousId);

    config.setPropertySelectionValue("AuthenticationMethod", StandardUserNamePasswordId);
    ASSERT_EQ(config.getSelectedAuthenticationMethod().getId(), StandardUserNamePasswordId);

    auto credentials = Dict<IString, IString>({{"UserName", "john"}, {"Password", "secret"}});
    config.setPropertyValue("SuppliedCredential", credentials);
    ASSERT_EQ(config.getSuppliedCredential(), credentials);

    ASSERT_THROW(config.setPropertyValue("SuppliedCredential", Dict<IString, IString>({{"UserName", "john"}})), InvalidParameterException);
}

TEST_F(AuthenticationConfigTest, SerializeDeserialize)
{
    const auto methods = AnonymousAndUserNamePasswordMethods();
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(methods);
    config.setAuthenticationMethod(StandardUserNamePasswordId);
    config.setSuppliedCredential(Dict<IString, IString>());

    const auto serializer = JsonSerializer();
    config.serialize(serializer);
    const auto serialized = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const AuthenticationConfigPtr newConfig = deserializer.deserialize(serialized, nullptr, nullptr);

    ASSERT_EQ(newConfig.getSelectedAuthenticationMethod().getId(), StandardUserNamePasswordId);
    const auto emptyCredentials = Dict<IString, IString>();
    ASSERT_EQ(newConfig.getSuppliedCredential(), emptyCredentials);

    // Deserialization rebuilds fresh `IAuthenticationMethod` instances rather than reusing the originals, and
    // `AuthenticationMethodImpl` has no value-based equality - compare each entry's own content instead.
    const auto deserializedMethods = newConfig.getAuthenticationMethods();
    const auto emptyFields = Dict<IString, ICredentialField>();
    ASSERT_EQ(deserializedMethods.getKeyList(), methods.getKeyList());
    ASSERT_EQ(deserializedMethods.get(StandardAnonymousId).getFields(), emptyFields);
    ASSERT_EQ(deserializedMethods.get(StandardUserNamePasswordId).getFields().getKeyList(), List<IString>("UserName", "Password"));

    serializer.reset();
    newConfig.serialize(serializer);
    ASSERT_EQ(serializer.getOutput(), serialized);
}

TEST_F(AuthenticationConfigTest, SerializeNeverWritesSuppliedCredential)
{
    const AuthenticationConfigPtr config = MakeAuthenticationConfig(AnonymousAndUserNamePasswordMethods());
    config.setAuthenticationMethod(StandardUserNamePasswordId);
    config.setSuppliedCredential(Dict<IString, IString>({{"UserName", "john"}, {"Password", "secret"}}));

    const auto serializer = JsonSerializer();
    config.serialize(serializer);

    const auto deserializer = JsonDeserializer();
    const AuthenticationConfigPtr newConfig = deserializer.deserialize(serializer.getOutput(), nullptr, nullptr);

    const auto emptyCredentials = Dict<IString, IString>();
    ASSERT_EQ(newConfig.getSuppliedCredential(), emptyCredentials);
}

END_NAMESPACE_OPENDAQ
