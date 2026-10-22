#include <gtest/gtest.h>
#include <opendaq/authentication_method_factory.h>
#include <opendaq/authentication_method_internal_ptr.h>

using AuthenticationMethodTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(AuthenticationMethodTest, FactoryNoFields)
{
    const auto emptyFields = Dict<IString, ICredentialField>();
    const AuthenticationMethodPtr method = AuthenticationMethod("AnonymousTest", emptyFields, "No credentials required");

    ASSERT_EQ(method.getId(), "AnonymousTest");
    ASSERT_EQ(method.getFields(), emptyFields);
    ASSERT_EQ(method.getDescription(), "No credentials required");
}

TEST_F(AuthenticationMethodTest, FactoryThrowsOnUnassignedParams)
{
    ASSERT_THROW(AuthenticationMethod(nullptr, Dict<IString, ICredentialField>(), "No credentials required"), InvalidParameterException);
    ASSERT_THROW(AuthenticationMethod("AnonymousTest", nullptr, "No credentials required"), InvalidParameterException);
    ASSERT_THROW(AuthenticationMethod("AnonymousTest", Dict<IString, ICredentialField>(), nullptr), InvalidParameterException);
}

TEST_F(AuthenticationMethodTest, FactoryWithFields)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")},
         {"Password", CredentialField("Password", CredentialFieldKind::Secret, "Password")}});

    const AuthenticationMethodPtr method = AuthenticationMethod("UserNamePasswordTest", fields, "Username and password");

    ASSERT_EQ(method.getId(), "UserNamePasswordTest");
    ASSERT_EQ(method.getDescription(), "Username and password");
    ASSERT_EQ(method.getFields(), fields);
}

TEST_F(AuthenticationMethodTest, SerializeDeserialize)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"Pin", CredentialField("Pin", CredentialFieldKind::Secret, "PIN code")}});
    const AuthenticationMethodPtr method = AuthenticationMethod("PinTest", fields, "PIN code");

    const auto serializer = JsonSerializer();
    method.serialize(serializer);
    const auto serialized = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const AuthenticationMethodPtr newMethod = deserializer.deserialize(serialized, nullptr, nullptr);

    ASSERT_EQ(newMethod.getId(), "PinTest");
    ASSERT_EQ(newMethod.getDescription(), "PIN code");
    ASSERT_EQ(newMethod.getFields().getKeyList(), List<IString>("Pin"));
    ASSERT_EQ(newMethod.getFields().get("Pin").getKind(), CredentialFieldKind::Secret);

    serializer.reset();
    newMethod.serialize(serializer);
    ASSERT_EQ(serializer.getOutput(), serialized);
}

TEST_F(AuthenticationMethodTest, StandardMethods)
{
    const auto anonymous = StandardAnonymousAuthenticationMethod();
    const auto emptyFields = Dict<IString, ICredentialField>();
    ASSERT_EQ(anonymous.getId(), StandardAnonymousId);
    ASSERT_EQ(anonymous.getFields(), emptyFields);

    const auto userNamePassword = StandardUserNamePasswordAuthenticationMethod();
    const auto userNamePasswordFields = userNamePassword.getFields();
    ASSERT_EQ(userNamePassword.getId(), StandardUserNamePasswordId);
    ASSERT_EQ(userNamePasswordFields.getKeyList(), List<IString>("UserName", "Password"));
    ASSERT_EQ(userNamePasswordFields.get("UserName").getKind(), CredentialFieldKind::Text);
    ASSERT_EQ(userNamePasswordFields.get("Password").getKind(), CredentialFieldKind::Secret);

    const auto pin = StandardPinAuthenticationMethod();
    const auto pinFields = pin.getFields();
    ASSERT_EQ(pin.getId(), StandardPinId);
    ASSERT_EQ(pinFields.getKeyList(), List<IString>("Pin"));
    ASSERT_EQ(pinFields.get("Pin").getKind(), CredentialFieldKind::Secret);

    const auto privateKeyFile = StandardPrivateKeyFileAuthenticationMethod();
    const auto privateKeyFileFields = privateKeyFile.getFields();
    ASSERT_EQ(privateKeyFile.getId(), StandardPrivateKeyFileId);
    ASSERT_EQ(privateKeyFileFields.getKeyList(), List<IString>("PrivateKeyFilePath"));
    const auto privateKeyField = privateKeyFileFields.get("PrivateKeyFilePath");
    const auto expectedPrivateKeyFileMetadata = Dict<IString, IString>({{"Extensions", "pem,key"}});
    ASSERT_EQ(privateKeyField.getKind(), CredentialFieldKind::FilePath);
    ASSERT_EQ(privateKeyField.getMetadata(), expectedPrivateKeyFileMetadata);
}

TEST_F(AuthenticationMethodTest, CredentialAcceptionThrowsOnUnassignedParams)
{
    ASSERT_THROW(StandardUserNamePasswordAuthenticationMethod().asPtr<IAuthenticationMethodInternal>().acceptsCredentials(nullptr),
                 ArgumentNullException);
}

TEST_F(AuthenticationMethodTest, CredentialAcceptionThrowsOnMethodWithNoFields)
{
    const auto method = StandardAnonymousAuthenticationMethod().asPtr<IAuthenticationMethodInternal>();

    ASSERT_THROW(method.acceptsCredentials(Dict<IString, IString>()), NotSupportedException);
    ASSERT_THROW(method.acceptsCredentials(Dict<IString, IString>({{"Whatever", "value"}})), NotSupportedException);
}

TEST_F(AuthenticationMethodTest, CredentialAcceptionByStandardMethod)
{
    const auto method = StandardUserNamePasswordAuthenticationMethod().asPtr<IAuthenticationMethodInternal>();

    ASSERT_FALSE(method.acceptsCredentials(Dict<IString, IString>({{"Password", "qwerty"}}))); // Missing Required Value
    ASSERT_FALSE(method.acceptsCredentials(Dict<IString, IString>({{"UserName", ""}, {"Password", "qwerty"}}))); // Empty Required Value
    ASSERT_TRUE(method.acceptsCredentials(Dict<IString, IString>({{"UserName", "john"}, {"Password", "qwerty"}}))); // All Required Fields Present
}

TEST_F(AuthenticationMethodTest, CredentialAcceptionNonRequiredFieldIgnored)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"UserName", CredentialField("UserName", CredentialFieldKind::Text, "User name")},
         {"Field", CredentialField("Field", CredentialFieldKind::Text, "Field", False)}});
    const auto method = AuthenticationMethod("CustomTest", fields, "Custom method").asPtr<IAuthenticationMethodInternal>();

    ASSERT_TRUE(method.acceptsCredentials(Dict<IString, IString>({{"UserName", "john"}})));
}

// A method whose fields are all optional accepts empty credential.
TEST_F(AuthenticationMethodTest, CredentialAcceptionAllFieldsOptional)
{
    const auto fields = Dict<IString, ICredentialField>(
        {{"Field", CredentialField("Field", CredentialFieldKind::Text, "Field", False)}});
    const auto method = AuthenticationMethod("AllOptionalTest", fields, "All fields optional").asPtr<IAuthenticationMethodInternal>();

    ASSERT_TRUE(method.acceptsCredentials(Dict<IString, IString>()));
    ASSERT_TRUE(method.acceptsCredentials(Dict<IString, IString>({{"Field", ""}})));
}

END_NAMESPACE_OPENDAQ
