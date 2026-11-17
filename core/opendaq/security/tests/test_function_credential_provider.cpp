#include <gtest/gtest.h>
#include <opendaq/credential_provider_factory.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/authentication_method_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/security_exceptions.h>
#include <coretypes/function_factory.h>
#include <testutils/testutils.h>

using FunctionCredentialProviderTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

static CredentialRequestPtr FunctionTestCredentialRequest()
{
    return CredentialRequestBuilder()
        .setComponentType(DeviceType("TestDeviceType", "Test device", "Test device type", "test"))
        .setConnectionString("daq://device1")
        .setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
        .build();
}

TEST_F(FunctionCredentialProviderTest, NullFunction)
{
    ASSERT_THROW(FunctionCredentialProvider(nullptr), ArgumentNullException);
}

TEST_F(FunctionCredentialProviderTest, GetDescription)
{
    StringPtr calledMethod;
    const auto provider = FunctionCredentialProvider(Function(
        [&](const StringPtr& methodName, const DictPtr<IString, IBaseObject>& params)
        {
            calledMethod = methodName;
            EXPECT_EQ(params.getCount(), 0u);
            return Dict<IString, IBaseObject>({{"Description", "Test provider"}});
        }));

    ASSERT_EQ(provider.getDescription(), "Test provider");
    ASSERT_EQ(calledMethod, "getDescription");
}

TEST_F(FunctionCredentialProviderTest, NullDescription)
{
    const auto provider = FunctionCredentialProvider(Function(
        [](const StringPtr&, const DictPtr<IString, IBaseObject>&) { return Dict<IString, IBaseObject>({{"Description", nullptr}}); }));

    const auto description = provider.getDescription();
    ASSERT_TRUE(description.assigned());
    ASSERT_EQ(description, "");
}

TEST_F(FunctionCredentialProviderTest, RequestCredentials)
{
    const auto request = FunctionTestCredentialRequest();
    StringPtr calledMethod;
    const auto provider = FunctionCredentialProvider(Function(
        [&](const StringPtr& methodName, const DictPtr<IString, IBaseObject>& params)
        {
            calledMethod = methodName;
            EXPECT_EQ(params.get("Request"), request);
            return Dict<IString, IBaseObject>({{"Credentials", Dict<IString, IString>({{"UserName", "user"}, {"Password", "secret"}})}});
        }));

    const auto credentials = provider.requestCredentials(request);
    const auto expectedCredentials = Dict<IString, IString>({{"UserName", "user"}, {"Password", "secret"}});

    ASSERT_EQ(credentials, expectedCredentials);
    ASSERT_EQ(calledMethod, "requestCredentials");
}

TEST_F(FunctionCredentialProviderTest, MissingResultKeys)
{
    const auto provider = FunctionCredentialProvider(Function(
        [](const StringPtr&, const DictPtr<IString, IBaseObject>&) { return Dict<IString, IBaseObject>(); }));

    ASSERT_THROW_MSG(provider.getDescription(), NotFoundException, "Description");
    ASSERT_THROW_MSG(provider.requestCredentials(FunctionTestCredentialRequest()), NotFoundException, "Credentials");
}

TEST_F(FunctionCredentialProviderTest, ErrorPropagation)
{
    const auto provider = FunctionCredentialProvider(Function(
        [](const StringPtr&, const DictPtr<IString, IBaseObject>&) -> BaseObjectPtr { DAQ_THROW_EXCEPTION(ProviderCancelledException); }));

    ASSERT_THROW(provider.requestCredentials(FunctionTestCredentialRequest()), ProviderCancelledException);
}

END_NAMESPACE_OPENDAQ
