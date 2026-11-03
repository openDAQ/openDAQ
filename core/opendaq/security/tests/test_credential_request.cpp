#include <gtest/gtest.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/authentication_method_factory.h>
#include <opendaq/device_type_factory.h>

using CredentialRequestTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

static ComponentTypePtr TestComponentType(const StringPtr& id = "TestDeviceType")
{
    return DeviceType(id, "Test device", "Test device type", "test");
}

TEST_F(CredentialRequestTest, BuilderSettersGetters)
{
    const auto componentType = TestComponentType();
    const auto method = StandardUserNamePasswordAuthenticationMethod();

    auto builder = CredentialRequestBuilder();
    builder.setComponentType(componentType);
    builder.setConnectionString("daq://device1");
    builder.setManufacturer("Acme");
    builder.setSerialNumber("12345");
    builder.setModel("ModelX");
    builder.setDisplayName("My device");
    builder.setAuthenticationMethod(method);

    ASSERT_EQ(builder.getComponentType().getId(), componentType.getId());
    ASSERT_EQ(builder.getConnectionString(), "daq://device1");
    ASSERT_EQ(builder.getManufacturer(), "Acme");
    ASSERT_EQ(builder.getSerialNumber(), "12345");
    ASSERT_EQ(builder.getModel(), "ModelX");
    ASSERT_EQ(builder.getDisplayName(), "My device");
    ASSERT_EQ(builder.getAuthenticationMethod().getId(), StandardUserNamePasswordId);
}

TEST_F(CredentialRequestTest, BuildThrowsWhenComponentTypeUnassigned)
{
    auto builder = CredentialRequestBuilder();
    builder.setConnectionString("daq://device1");
    builder.setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod());

    ASSERT_THROW(builder.build(), InvalidParameterException);
}

TEST_F(CredentialRequestTest, BuildThrowsWhenConnectionStringUnassigned)
{
    auto builder = CredentialRequestBuilder();
    builder.setComponentType(TestComponentType());
    builder.setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod());

    ASSERT_THROW(builder.build(), InvalidParameterException);
}

TEST_F(CredentialRequestTest, BuildThrowsWhenConnectionStringEmpty)
{
    auto builder = CredentialRequestBuilder();
    builder.setComponentType(TestComponentType());
    builder.setConnectionString("");
    builder.setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod());

    ASSERT_THROW(builder.build(), InvalidParameterException);
}

TEST_F(CredentialRequestTest, BuildThrowsWhenAuthenticationMethodUnassigned)
{
    auto builder = CredentialRequestBuilder();
    builder.setComponentType(TestComponentType());
    builder.setConnectionString("daq://device1");

    ASSERT_THROW(builder.build(), InvalidParameterException);
}

TEST_F(CredentialRequestTest, BuildSucceedsWithOnlyRequiredFields)
{
    const auto componentType = TestComponentType();
    const auto method = StandardUserNamePasswordAuthenticationMethod();

    auto builder = CredentialRequestBuilder();
    builder.setComponentType(componentType);
    builder.setConnectionString("daq://device1");
    builder.setAuthenticationMethod(method);

    const auto request = builder.build();

    ASSERT_EQ(request.getComponentType().getId(), componentType.getId());
    ASSERT_EQ(request.getConnectionString(), "daq://device1");
    ASSERT_EQ(request.getAuthenticationMethod().getId(), StandardUserNamePasswordId);
    ASSERT_FALSE(request.getManufacturer().assigned());
    ASSERT_FALSE(request.getSerialNumber().assigned());
    ASSERT_FALSE(request.getModel().assigned());
    ASSERT_FALSE(request.getDisplayName().assigned());
}

TEST_F(CredentialRequestTest, RequestCarriesBuilderValues)
{
    const auto componentType = TestComponentType();
    const auto method = StandardPinAuthenticationMethod();

    auto builder = CredentialRequestBuilder();
    builder.setComponentType(componentType);
    builder.setConnectionString("daq://device1");
    builder.setManufacturer("Acme");
    builder.setSerialNumber("12345");
    builder.setModel("ModelX");
    builder.setDisplayName("My device");
    builder.setAuthenticationMethod(method);

    const auto request = builder.build();

    ASSERT_EQ(request.getComponentType().getId(), componentType.getId());
    ASSERT_EQ(request.getConnectionString(), "daq://device1");
    ASSERT_EQ(request.getManufacturer(), "Acme");
    ASSERT_EQ(request.getSerialNumber(), "12345");
    ASSERT_EQ(request.getModel(), "ModelX");
    ASSERT_EQ(request.getDisplayName(), "My device");
    ASSERT_EQ(request.getAuthenticationMethod().getId(), StandardPinId);
}

END_NAMESPACE_OPENDAQ
