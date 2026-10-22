#include "test_helpers/test_helpers.h"
#include <credential_demo_module/module_dll.h>
#include <opendaq/authentication_method_factory.h>
#include <opendaq/security_exceptions.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/device_update_options_factory.h>
#include <opendaq/update_parameters_factory.h>

using namespace daq;

// Exercises the credential demo module through the full stack it was built to showcase:
// - the app layer (`InstanceBuilder`/`Instance::addDevice`/`addStreaming`),
// - the module manager to module layer,
// - the credential provider layer (`ICredentialProvider`) -
// rather than calling the demo-module's own `createAuthenticatedDevice`/`createAuthenticatedStreaming` directly,
// which `examples/modules/credential_demo_module/tests/test_credential_demo_module.cpp` already covers.

// Mirrors the app-layer bridge in `credential_providers.cpp` - `IAuthenticationConfig` isn't yet a first-class
// part of the add-device/add-streaming config schema, so it's smuggled onto an otherwise plain config property
// object under this key, which `ModuleManagerImpl` looks for and honors (see `AuthenticationConfigConfigKey`/
// `ExtractAuthenticationConfig` in `module_manager_impl.cpp`).
static const char* AuthenticationConfigConfigKey = "__AuthenticationConfig";

static PropertyObjectPtr WithAuthenticationConfig(const AuthenticationConfigPtr& authConfig)
{
    auto config = PropertyObject();
    config.addProperty(ObjectProperty(AuthenticationConfigConfigKey, authConfig));
    return config;
}

// The simplest `ICredentialProvider` - returns the one dictionary it was given at construction time,
// regardless of what's being requested, unless `errorCode` is set to something other than `OPENDAQ_SUCCESS`,
// in which case it fails with that error code instead (e.g. `OPENDAQ_ERR_PROVIDER_CANCELLED` for a cancelled
// prompt, or some other, unrelated, generic provider failure).
class TestCredentialProviderImpl final : public ImplementationOf<ICredentialProvider>
{
public:
    explicit TestCredentialProviderImpl(DictPtr<IString, IString> credentials, ErrCode errorCode = OPENDAQ_SUCCESS)
        : credentials(std::move(credentials))
        , errorCode(errorCode)
    {
    }

    ErrCode INTERFACE_FUNC getDescription(IString** description) override
    {
        OPENDAQ_PARAM_NOT_NULL(description);
        *description = String("Test credential provider").detach();
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC requestCredentials(ICredentialRequest* /*request*/, IDict** credentialsOut) override
    {
        if (OPENDAQ_FAILED(errorCode))
            return DAQ_MAKE_ERROR_INFO(errorCode, "Simulated credential provider failure");

        OPENDAQ_PARAM_NOT_NULL(credentialsOut);
        *credentialsOut = credentials.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

private:
    DictPtr<IString, IString> credentials;
    ErrCode errorCode;
};

static CredentialProviderPtr TestCredentialProvider(const DictPtr<IString, IString>& credentials, ErrCode errorCode = OPENDAQ_SUCCESS)
{
    return createWithImplementation<ICredentialProvider, TestCredentialProviderImpl>(credentials, errorCode);
}

class CredentialDemoModuleIntegrationTest : public testing::Test
{
public:
    static constexpr const char* DeviceConnectionString = "daq.credential_demo://credential_demo_device";
    static constexpr const char* StreamingConnectionString = "daq.credential_demo_streaming://credential_demo_device";

    InstancePtr createClientInstance(const CredentialProviderPtr& credentialProvider = nullptr)
    {
        auto builder = test_helpers::instanceBuilder().setModulePath("[[none]]");
        if (credentialProvider.assigned())
            builder.setCredentialProvider(credentialProvider);
        auto instance = InstanceFromBuilder(builder);

        ModulePtr credentialDemoModule;
        createCredentialDemoModule(&credentialDemoModule, instance.getContext());
        instance.getModuleManager().addModule(credentialDemoModule);

        return instance;
    }
};

TEST_F(CredentialDemoModuleIntegrationTest, AddDeviceSucceedsWithProviderSuppliedCredentials)
{
    const auto provider = TestCredentialProvider(Dict<IString, IString>({{"UserName", "user"}, {"Password", "pass"}}));
    auto instance = createClientInstance(provider);

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    DevicePtr device;
    ASSERT_NO_THROW(device = instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig)));
    ASSERT_TRUE(device.assigned());
}

TEST_F(CredentialDemoModuleIntegrationTest, AddDeviceThrowsWhenProviderSuppliesWrongCredentials)
{
    const auto provider = TestCredentialProvider(Dict<IString, IString>({{"UserName", "user"}, {"Password", "wrong"}}));
    auto instance = createClientInstance(provider);

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    ASSERT_THROW(instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig)), AuthenticationFailedException);
}

TEST_F(CredentialDemoModuleIntegrationTest, AddDeviceThrowsWhenNoProviderRegisteredAndCredentialsRequired)
{
    auto instance = createClientInstance(); // No credential provider registered.

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    ASSERT_THROW(instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig)), CredentialsRequiredException);
}

TEST_F(CredentialDemoModuleIntegrationTest, AddDeviceWithSuppliedCredentialBypassesProvider)
{
    auto instance = createClientInstance(); // No credential provider registered - must not be needed.

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");
    authConfig.setSuppliedCredential(Dict<IString, IString>({{"UserName", "user"}, {"Password", "pass"}}));

    DevicePtr device;
    ASSERT_NO_THROW(device = instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig)));
    ASSERT_TRUE(device.assigned());
}

TEST_F(CredentialDemoModuleIntegrationTest, SetSuppliedCredentialThrowsWhenIncomplete)
{
    auto instance = createClientInstance(); // No credential provider registered.

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    // Missing the "Password" field `UserNamePassword` requires - `AuthenticationConfigImpl::setPropertyValue`
    // validates a supplied credential against the selected method's `acceptsCredentials` immediately, right
    // here, rather than deferring that check to `addDevice`/`resolveCredentials`.
    ASSERT_THROW(authConfig.setSuppliedCredential(Dict<IString, IString>({{"UserName", "user"}})), InvalidParameterException);
}

TEST_F(CredentialDemoModuleIntegrationTest, AddDeviceThrowsWhenProviderCancels)
{
    const auto provider = TestCredentialProvider(Dict<IString, IString>(), OPENDAQ_ERR_PROVIDER_CANCELLED);
    auto instance = createClientInstance(provider);

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    ASSERT_THROW(instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig)), ProviderCancelledException);
}

TEST_F(CredentialDemoModuleIntegrationTest, AddDeviceThrowsWhenProviderFailsWithGenericError)
{
    const auto provider = TestCredentialProvider(Dict<IString, IString>(), OPENDAQ_ERR_GENERALERROR);
    auto instance = createClientInstance(provider);

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    ASSERT_THROW(instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig)), GeneralErrorException);
}

TEST_F(CredentialDemoModuleIntegrationTest, AddDeviceThrowsWhenProviderSuppliesIncompleteCredentials)
{
    // Missing the "Password" field `UserNamePassword` requires - `acceptsCredentials` rejects it.
    const auto provider = TestCredentialProvider(Dict<IString, IString>({{"UserName", "user"}}));
    auto instance = createClientInstance(provider);

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    ASSERT_THROW(instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig)), CredentialsRequiredException);
}

TEST_F(CredentialDemoModuleIntegrationTest, AddStreamingSucceedsAnonymously)
{
    auto instance = createClientInstance(); // No credential provider registered - must not be needed.

    auto device = instance.addDevice(DeviceConnectionString); // Plain, anonymous device connection.

    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = device.addStreaming(StreamingConnectionString)); // Plain, anonymous streaming connection.
    ASSERT_TRUE(streaming.assigned());
}

TEST_F(CredentialDemoModuleIntegrationTest, AddStreamingSucceedsWithProviderSuppliedCredentials)
{
    const auto provider = TestCredentialProvider(Dict<IString, IString>({{"UserName", "user"}, {"Password", "pass"}}));
    auto instance = createClientInstance(provider);

    auto device = instance.addDevice(DeviceConnectionString); // Plain, anonymous device connection.

    const auto streamingType = instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().get("CredentialDemoStreaming");
    auto streamingAuthConfig = streamingType.createAuthenticationConfig();
    streamingAuthConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = device.addStreaming(StreamingConnectionString, WithAuthenticationConfig(streamingAuthConfig)));
    ASSERT_TRUE(streaming.assigned());
}

TEST_F(CredentialDemoModuleIntegrationTest, AddStreamingThrowsWhenProviderSuppliesWrongCredentials)
{
    const auto provider = TestCredentialProvider(Dict<IString, IString>({{"UserName", "user"}, {"Password", "wrong"}}));
    auto instance = createClientInstance(provider);

    auto device = instance.addDevice(DeviceConnectionString); // Plain, anonymous device connection.

    const auto streamingType = instance.getModuleManager().asPtr<IModuleManagerUtils>().getAvailableStreamingTypes().get("CredentialDemoStreaming");
    auto streamingAuthConfig = streamingType.createAuthenticationConfig();
    streamingAuthConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");

    ASSERT_THROW(device.addStreaming(StreamingConnectionString, WithAuthenticationConfig(streamingAuthConfig)), AuthenticationFailedException);
}

TEST_F(CredentialDemoModuleIntegrationTest, AuthenticationSettingsViaUpdateOptionsSucceeds)
{
    auto instance = createClientInstance(); // No credential provider registered.

    // Saved authenticated with UserNamePassword.
    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");
    authConfig.setSuppliedCredential(Dict<IString, IString>({{"UserName", "user"}, {"Password", "pass"}}));

    auto device = instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig));
    ASSERT_TRUE(device.assigned());
    const auto localId = device.getLocalId();

    const auto str = instance.saveConfiguration();
    instance.removeDevice(device);
    ASSERT_EQ(instance.getDevices().getCount(), 0u);

    const auto options = DeviceUpdateOptions(str);
    const auto savedAuthConfig = options.getChildDeviceOptions()[0].getAuthenticationConfig();
    ASSERT_TRUE(savedAuthConfig.assigned());
    ASSERT_EQ(savedAuthConfig.getSelectedAuthenticationMethod().getId(), "UserNamePassword");

    savedAuthConfig.setAuthenticationMethod("Pin");
    savedAuthConfig.setSuppliedCredential(Dict<IString, IString>({{"Pin", "1234"}}));

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);

    ASSERT_NO_THROW(instance.loadConfiguration(str, params));
    ASSERT_EQ(instance.getDevices().getCount(), 1u);
    ASSERT_EQ(instance.getDevices()[0].getLocalId(), localId);
}

TEST_F(CredentialDemoModuleIntegrationTest, AuthenticationSettingsViaUpdateOptionsWithWrongCredentialFails)
{
    auto instance = createClientInstance(); // No credential provider registered.

    const auto deviceType = instance.getAvailableDeviceTypes().get("CredentialDemoDevice");
    auto authConfig = deviceType.createAuthenticationConfig();
    authConfig.setPropertySelectionValue("AuthenticationMethod", "UserNamePassword");
    authConfig.setSuppliedCredential(Dict<IString, IString>({{"UserName", "user"}, {"Password", "pass"}}));

    auto device = instance.addDevice(DeviceConnectionString, WithAuthenticationConfig(authConfig));
    ASSERT_TRUE(device.assigned());

    const auto str = instance.saveConfiguration();
    instance.removeDevice(device);

    const auto options = DeviceUpdateOptions(str);
    const auto savedAuthConfig = options.getChildDeviceOptions()[0].getAuthenticationConfig();
    savedAuthConfig.setAuthenticationMethod("Pin");
    savedAuthConfig.setSuppliedCredential(Dict<IString, IString>({{"Pin", "0000"}})); // Wrong PIN.

    auto params = UpdateParameters();
    params.setDeviceUpdateOptions(options);

    // A per-device reload failure (AuthenticationFailedException here, from the module's own PIN check) is
    // caught and logged - loadConfiguration itself does not throw, the device is simply absent afterward.
    ASSERT_NO_THROW(instance.loadConfiguration(str, params));
    ASSERT_EQ(instance.getDevices().getCount(), 0u);
}
