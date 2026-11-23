#include <iostream>
#include <fstream>
#include <opendaq/opendaq.h>

using namespace daq;

static const std::string JSON_CONFIG_FILE_NAME = "credential-demo-opendaq-config.json";

void createJsonConfigFile()
{
    std::string filename = JSON_CONFIG_FILE_NAME;
    std::string options = R"(
    {
    "Modules": {
        "CredentialDemoModule": {
            "Manufacturer": "openDAQ",
            "SerialNumber": "1234",
            "PublicKeyPath": ")" + std::string(CREDENTIAL_DEMO_KEYS_DIR) + R"(/public_key.pem"
            }
        }
    }
    )";

    std::ofstream file;
    file.open(filename);
    if (!file.is_open())
        throw std::runtime_error("can not open file for writing");

    file << options;
    file.close();
}

static const char* DeviceConnectionString = "daq://openDAQ_1234";

// An add-device config for the discovered credential demo device. Its authentication config -
// `config.getAuthenticationConfig()` - and that of its streaming config are the ones used when the device is added.
AddDeviceConfigPtr CreateAddDeviceConfig(const InstancePtr& instance)
{
    for (const auto& info : instance.getAvailableDevices())
    {
        if (info.getConnectionString() == DeviceConnectionString)
            return instance.createAddDeviceConfig(info);
    }
    throw std::runtime_error("The credential demo device was not discovered");
}

// An authentication config lists every authentication method the component type supports (Anonymous,
// UserNamePassword, Pin, and - when the module's private-key auth option is on - PrivateKeyFile) as a candidate
// of its "AuthenticationMethod" selection property, defaulting to Anonymous, the first one added - so every
// demo below that wants a specific method selects it explicitly rather than relying on the default.
// This switches that selection to the method named by `authenticationMethodId`, entirely through
// plain property object calls - the candidates are already just ids, so no `IAuthenticationMethod` lookup
// is needed at all, only a generic selection write.
void SelectAuthenticationMethod(const AuthenticationConfigPtr& authConfig, const StringPtr& authenticationMethodId)
{
    authConfig.asPtr<IPropertyObject>().setPropertySelectionValue("AuthenticationMethod", authenticationMethodId);
}

// Not every device/streaming type supports every authentication method - in particular, the credential demo
// module's `StandardPrivateKeyFile` support is itself a build-time option (`DAQMODULES_CREDENTIAL_DEMO_MODULE_
// ENABLE_PRIVATE_KEY_AUTH`), off by default. Checked the same way `SelectAuthenticationMethod` sets the
// selection itself - via the "AuthenticationMethod" selection property's candidates - rather than assuming a
// method id is always valid to select.
bool SupportsAuthenticationMethod(const AuthenticationConfigPtr& authConfig, const StringPtr& authenticationMethodId)
{
    ListPtr<IString> candidates = authConfig.asPtr<IPropertyObject>().getProperty("AuthenticationMethod").getSelectionValues();
    for (const auto& candidate : candidates)
        if (candidate == authenticationMethodId)
            return true;
    return false;
}

// PrivateKeyFile authentication - a single FilePath-kind field, but instead of comparing a
// fixed value, the module verifies a signed challenge against the public key configured via the
// "PublicKeyPath" module option (set above to keys/public_key.pem). When prompted, supply the path
// to the matching private key.
void demoPrivateKeyFileAuthentication(const InstancePtr& instance)
{
    auto config = CreateAddDeviceConfig(instance);
    auto privateKeyFileConfig = config.getAuthenticationConfig();
    if (!SupportsAuthenticationMethod(privateKeyFileConfig, "PrivateKeyFile"))
    {
        std::cout << "PrivateKeyFile authentication is disabled in this build of the credential demo module - skipping." << std::endl;
        return;
    }

    std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    SelectAuthenticationMethod(privateKeyFileConfig, "PrivateKeyFile");
    auto device = instance.addDevice(DeviceConnectionString, config);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with private-key challenge authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// add without authentication
void demoNoAuthentication(const InstancePtr& instance)
{
    auto device = instance.addDevice(DeviceConnectionString);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" without authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// authenticate with username and password - a Text field and a Secret field.
void demoUserNamePasswordAuthentication(const InstancePtr& instance)
{
    auto config = CreateAddDeviceConfig(instance);
    SelectAuthenticationMethod(config.getAuthenticationConfig(), "UserNamePassword");
    auto device = instance.addDevice(DeviceConnectionString, config);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication. Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// `IAuthenticationConfig` derives from `IPropertyObject` - the same way `IDeviceInfo` does - so besides the
// typed getters used in every other demo, the exact same settings can be read and set with plain property
// object calls instead. `config.getAuthenticationConfig()` already returns one self-contained config with
// every supported method (Anonymous, UserNamePassword, Pin, and - when the module's private-key auth option
// is on - PrivateKeyFile) as a candidate of its "AuthenticationMethod" selection property -
// `SelectAuthenticationMethod` switches that selection to
// "Pin" via plain property object calls, no separate per-method config fetched anywhere.
void demoAuthenticationConfigAsPropertyObject(const InstancePtr& instance)
{
    auto config = CreateAddDeviceConfig(instance);
    auto authConfig = config.getAuthenticationConfig();
    SelectAuthenticationMethod(authConfig, "Pin");

    StringPtr authenticationMethodId = authConfig.asPtr<IPropertyObject>().getPropertySelectionValue("AuthenticationMethod");
    std::cout << "Authentication method id, read as a plain property object selection value: " << authenticationMethodId << std::endl;

    std::cout << "When prompted for the PIN, enter: 1234" << std::endl;
    auto device = instance.addDevice(DeviceConnectionString, config);
    std::cout << "Connected to \"" << device.getInfo().getName()
              << "\" with PIN authentication, configured entirely via plain property object calls on the authentication config itself. "
                 "Press \"enter\" to continue..."
              << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// The device and the streaming connection attached to it are authenticated entirely independently of one
// another - the device here via UserName/Password, the streaming connection via its own, separate
// authentication config (PrivateKeyFile), set on the streaming config nested in the add-device config.
void demoDeviceAndStreamingAuthentication(const InstancePtr& instance)
{
    auto config = CreateAddDeviceConfig(instance);
    SelectAuthenticationMethod(config.getAuthenticationConfig(), "UserNamePassword");

    auto streamingAuthConfig = config.getStreamingOptions().getStreamingConfig().getAuthenticationConfig();
    if (SupportsAuthenticationMethod(streamingAuthConfig, "PrivateKeyFile"))
    {
        SelectAuthenticationMethod(streamingAuthConfig, "PrivateKeyFile");
        std::cout << "When prompted for the private-key path, enter: " << CREDENTIAL_DEMO_KEYS_DIR << "/private_key.pem" << std::endl;
    }
    else
    {
        std::cout << "PrivateKeyFile authentication is disabled in this build of the credential demo module - "
                     "falling back to Anonymous for the streaming connection."
                  << std::endl;
    }

    std::cout << "Device authentication (UserName/Password):" << std::endl;
    auto device = instance.addDevice(DeviceConnectionString, config);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with UserName/Password authentication." << std::endl;
    std::cout << "Attached a streaming connection authenticated independently of the device. Streaming sources: "
              << device.asPtr<IMirroredDevice>().getStreamingSources().getCount() << std::endl;
    std::cout << "Press \"enter\" to continue..." << std::endl;
    std::cin.get();
    instance.removeDevice(device);
}

// PIN authentication - a single Secret-kind field. The device is authenticated via
// PIN. Finally, the instance is saved and reloaded into a completely separate instance to show that a
// previously authenticated device is re-authenticated (not silently reconnected) on load.
void demoPinAuthenticationAndReload(const InstancePtr& instance)
{
    auto config = CreateAddDeviceConfig(instance);
    SelectAuthenticationMethod(config.getAuthenticationConfig(), "Pin");
    auto device = instance.addDevice(DeviceConnectionString, config);
    std::cout << "Connected to \"" << device.getInfo().getName() << "\" with PIN authentication." << std::endl;

    std::cout << "Press \"enter\" to save the configuration and reload it into a new instance..." << std::endl;
    std::cin.get();

    // Saving the instance carries the connected device's authentication config along with it, but only in
    // reduced form - every candidate authentication method and the selected one's id - never a supplied or
    // obtained credential.
    auto savedConfiguration = instance.saveConfiguration();

    // A completely separate instance, loading the saved configuration - it needs its own credential provider
    // registered, since the reloaded device is re-authenticated (the provider is asked for real credentials
    // again) rather than silently reconnected without any.
    auto reloadedCredentialProvider = CmdLineCredentialProvider();

    auto reloadedInstanceBuilder = InstanceBuilder();
    reloadedInstanceBuilder.addModulePath(MODULE_PATH);
    reloadedInstanceBuilder.addConfigProvider(JsonConfigProvider(JSON_CONFIG_FILE_NAME));
    reloadedInstanceBuilder.setCredentialProvider(reloadedCredentialProvider);
    auto reloadedInstance = reloadedInstanceBuilder.build();

    reloadedInstance.loadConfiguration(savedConfiguration);

    auto reloadedDevices = reloadedInstance.getDevices();
    if (reloadedDevices.getCount() == 0)
        throw std::runtime_error("Reloaded instance has no devices - the device failed to reconnect on load");

    auto reloadedDevice = reloadedDevices[0];
    std::cout << "Reloaded instance re-authenticated and reconnected to \"" << reloadedDevice.getInfo().getName() << std::endl;
}

int main(int /*argc*/, const char* /*argv*/[])
{
    createJsonConfigFile();

    // Credential provider can ever be registered on an instance `IInstanceBuilder::setCredentialProvider` -
    // `CmdLineCredentialProvider` supports every field kind this demo needs (Text, Secret, FilePath).
    auto credentialProvider = CmdLineCredentialProvider();

    auto instanceBuilder = InstanceBuilder();
    instanceBuilder.addModulePath(MODULE_PATH);
    instanceBuilder.addConfigProvider(JsonConfigProvider(JSON_CONFIG_FILE_NAME));
    instanceBuilder.setCredentialProvider(credentialProvider);
    auto instance = instanceBuilder.build();

    // demoPrivateKeyFileAuthentication(instance);
    // demoNoAuthentication(instance);
    demoUserNamePasswordAuthentication(instance);
    demoDeviceAndStreamingAuthentication(instance);
    demoAuthenticationConfigAsPropertyObject(instance);
    demoPinAuthenticationAndReload(instance);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
