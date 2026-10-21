#include <gtest/gtest.h>
#include <opendaq/credential_field_factory.h>
#include <coretypes/dictobject_factory.h>

using CredentialFieldTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(CredentialFieldTest, FactoryDefaults)
{
    const CredentialFieldPtr field = CredentialField("Username", CredentialFieldKind::Text, "User name");
    const auto emptyMetadata = Dict<IString, IString>();

    ASSERT_EQ(field.getId(), "Username");
    ASSERT_EQ(field.getKind(), CredentialFieldKind::Text);
    ASSERT_EQ(field.getName(), "User name");
    ASSERT_EQ(field.getMetadata(), emptyMetadata);
    ASSERT_TRUE(field.isRequired());
}

TEST_F(CredentialFieldTest, FactoryWithParams)
{
    const auto metadata = Dict<IString, IString>({{"Extensions", "pem,key"}});

    const CredentialFieldPtr field =
        CredentialField("PrivateKeyFilePath", CredentialFieldKind::FilePath, "Private key file", False, metadata);

    ASSERT_EQ(field.getId(), "PrivateKeyFilePath");
    ASSERT_EQ(field.getKind(), CredentialFieldKind::FilePath);
    ASSERT_EQ(field.getName(), "Private key file");
    ASSERT_EQ(field.getMetadata(), metadata);
    ASSERT_FALSE(field.isRequired());
}

TEST_F(CredentialFieldTest, FactoryThrowsOnUnassignedParams)
{
    ASSERT_THROW(CredentialField(nullptr, CredentialFieldKind::Text, "User name"), InvalidParameterException);
    ASSERT_THROW(CredentialField("Username", CredentialFieldKind::Text, nullptr), InvalidParameterException);
    ASSERT_THROW(CredentialField("Username", CredentialFieldKind::Text, "User name", True, nullptr), InvalidParameterException);
}

TEST_F(CredentialFieldTest, SecretKind)
{
    const CredentialFieldPtr field = CredentialField("Password", CredentialFieldKind::Secret, "Password", False);

    ASSERT_EQ(field.getKind(), CredentialFieldKind::Secret);
    ASSERT_FALSE(field.isRequired());
}

TEST_F(CredentialFieldTest, SerializeDeserializeWithMetadata)
{
    const auto metadata = Dict<IString, IString>({{"Extensions", "pem,key"}});
    const CredentialFieldPtr field =
        CredentialField("PrivateKeyFilePath", CredentialFieldKind::FilePath, "Private key file", False, metadata);

    const auto serializer = JsonSerializer();
    field.serialize(serializer);
    const auto serialized = serializer.getOutput();

    const auto deserializer = JsonDeserializer();
    const CredentialFieldPtr newField = deserializer.deserialize(serialized, nullptr, nullptr);

    ASSERT_EQ(newField.getId(), "PrivateKeyFilePath");
    ASSERT_EQ(newField.getKind(), CredentialFieldKind::FilePath);
    ASSERT_EQ(newField.getName(), "Private key file");
    ASSERT_EQ(newField.getMetadata(), metadata);
    ASSERT_FALSE(newField.isRequired());

    serializer.reset();
    newField.serialize(serializer);
    ASSERT_EQ(serializer.getOutput(), serialized);
}

class CredentialFieldAllKindsTest : public CredentialFieldTest,
                                     public testing::WithParamInterface<std::tuple<CredentialFieldKind, bool>>
{
};

TEST_P(CredentialFieldAllKindsTest, SerializeDeserialize)
{
    const auto kind = std::get<0>(GetParam());
    const Bool required = std::get<1>(GetParam());
    const CredentialFieldPtr field = CredentialField("Id", kind, "Name", required);

    const auto serializer = JsonSerializer();
    field.serialize(serializer);

    const auto deserializer = JsonDeserializer();
    const CredentialFieldPtr newField = deserializer.deserialize(serializer.getOutput(), nullptr, nullptr);

    ASSERT_EQ(newField.getKind(), kind);
    ASSERT_EQ(newField.isRequired(), required);
}

INSTANTIATE_TEST_SUITE_P(
    AllKinds,
    CredentialFieldAllKindsTest,
    testing::Values(
        std::make_tuple(CredentialFieldKind::Text, true),
        std::make_tuple(CredentialFieldKind::Secret, false)
    )
);

END_NAMESPACE_OPENDAQ
