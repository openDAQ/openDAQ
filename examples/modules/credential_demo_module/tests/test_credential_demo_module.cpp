#include <testutils/testutils.h>
#include <credential_demo_module/module_dll.h>
#include <credential_demo_module/version.h>

#include <opendaq/module_ptr.h>
#include <coretypes/common.h>
#include <coretypes/dictobject_factory.h>

#include <opendaq/context_factory.h>
#include <opendaq/login_parameters_factory.h>
#include <opendaq/authentication_method_factory.h>
#include <coreobjects/exceptions.h>
#include <cstdio>
#include <fstream>

using CredentialDemoModuleTest = testing::Test;
using namespace daq;

static constexpr const char* DeviceConnectionString = "daq.credential_demo://credential_demo_device";
static constexpr const char* StreamingConnectionString = "daq.credential_demo_streaming://credential_demo_device";

#ifdef DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH

// A valid, matching EC (P-256) key pair, and a second, mismatched private key, generated once via
// `openssl genpkey`/`openssl pkey -pubout` - used to exercise `StandardPrivateKeyFile` authentication
// without depending on any key material outside this file.
static constexpr const char* MatchingPublicKeyPem =
    "-----BEGIN PUBLIC KEY-----\n"
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAECq3LvV1nA9DNk3IReHc47O63dmvE\n"
    "oAZ3f5EU5/Rhz9jwPnPBVbmO9u17RXu5a0v7m2cox0Tp+pg901dO3D1naQ==\n"
    "-----END PUBLIC KEY-----\n";

static constexpr const char* MatchingPrivateKeyPem =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgS2rrsMDRzQVXyFbK\n"
    "WimjI76H6GzpReVJDyAETgHSIGChRANCAAQKrcu9XWcD0M2TchF4dzjs7rd2a8Sg\n"
    "Bnd/kRTn9GHP2PA+c8FVuY727XtFe7lrS/ubZyjHROn6mD3TV07cPWdp\n"
    "-----END PRIVATE KEY-----\n";

static constexpr const char* MismatchedPrivateKeyPem =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgd7B4KeehH7XzjkvM\n"
    "kxF++GCHvJeetcTn41L3bfiyW/6hRANCAARqGDUvTKTXNqEKD9JHK1chiIObMbwb\n"
    "EKE8XsjDNTCfb50div4gg3v68pvIcQzd1DGbCufO9QLCVrAOptS+N//u\n"
    "-----END PRIVATE KEY-----\n";

// A temporary file that exists for the lifetime of the object - used to supply key material as a
// `PrivateKeyFilePath` credential or a `PublicKeyPath` module option.
class CredentialDemoTestTempFile
{
public:
    CredentialDemoTestTempFile(const std::string& path, const std::string& content)
        : path(path)
    {
        std::ofstream(path) << content;
    }

    ~CredentialDemoTestTempFile()
    {
        std::remove(path.c_str());
    }

    const std::string path;
};

#endif // DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH

static ContextPtr contextWithModuleOptions(const DictPtr<IString, IBaseObject>& moduleOptions)
{
    auto options = Dict<IString, IBaseObject>();
    if (moduleOptions.assigned() && moduleOptions.getCount() > 0)
    {
        auto modules = Dict<IString, IBaseObject>();
        modules.set("CredentialDemoModule", moduleOptions);
        options.set("Modules", modules);
    }

    return NullContext(Logger(), TypeManager(), options);
}

static ModulePtr createModule(const ContextPtr& context = NullContext())
{
    ModulePtr module;
    createModule(&module, context);
    return module;
}

TEST_F(CredentialDemoModuleTest, CreateModule)
{
    IModule* module = nullptr;
    ErrCode errCode = createModule(&module, NullContext());
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(errCode));

    ASSERT_NE(module, nullptr);
    module->releaseRef();
}

TEST_F(CredentialDemoModuleTest, ModuleName)
{
    auto module = createModule();
    ASSERT_EQ(module.getModuleInfo().getName(), "Credential demo module");
}

TEST_F(CredentialDemoModuleTest, ModuleId)
{
    auto module = createModule();
    ASSERT_EQ(module.getModuleInfo().getId(), "CredentialDemoModule");
}

TEST_F(CredentialDemoModuleTest, VersionAvailable)
{
    auto module = createModule();
    ASSERT_TRUE(module.getModuleInfo().getVersionInfo().assigned());
}

TEST_F(CredentialDemoModuleTest, VersionCorrect)
{
    auto module = createModule();
    auto version = module.getModuleInfo().getVersionInfo();

    ASSERT_EQ(version.getMajor(), CREDENTIAL_DEMO_MODULE_MAJOR_VERSION);
    ASSERT_EQ(version.getMinor(), CREDENTIAL_DEMO_MODULE_MINOR_VERSION);
    ASSERT_EQ(version.getPatch(), CREDENTIAL_DEMO_MODULE_PATCH_VERSION);
}

TEST_F(CredentialDemoModuleTest, EnumerateDevices)
{
    auto module = createModule();

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
    ASSERT_EQ(deviceInfo.getCount(), 1u);

    ASSERT_EQ(deviceInfo[0].getManufacturer(), "openDAQ");
    ASSERT_EQ(deviceInfo[0].getSerialNumber(), "0");
    ASSERT_EQ(deviceInfo[0].getConnectionString(), "daq.credential_demo://credential_demo_device");
}

TEST_F(CredentialDemoModuleTest, EnumerateDevicesWithModuleOptions)
{
    auto moduleOptions = Dict<IString, IBaseObject>({{"Manufacturer", "TestManufacturer"}, {"SerialNumber", "42"}});
    auto module = createModule(contextWithModuleOptions(moduleOptions));

    ListPtr<IDeviceInfo> deviceInfo;
    ASSERT_NO_THROW(deviceInfo = module.getAvailableDevices());
    ASSERT_EQ(deviceInfo.getCount(), 1u);

    ASSERT_EQ(deviceInfo[0].getManufacturer(), "TestManufacturer");
    ASSERT_EQ(deviceInfo[0].getSerialNumber(), "42");
    ASSERT_EQ(deviceInfo[0].getConnectionString(), "daq.credential_demo://credential_demo_device");
}

TEST_F(CredentialDemoModuleTest, GetAvailableDeviceTypes)
{
    auto module = createModule();

    DictPtr<IString, IDeviceType> deviceTypes;
    ASSERT_NO_THROW(deviceTypes = module.getAvailableDeviceTypes());
    ASSERT_EQ(deviceTypes.getCount(), 1u);
    ASSERT_TRUE(deviceTypes.hasKey("CredentialDemoDevice"));
}

TEST_F(CredentialDemoModuleTest, GetAvailableStreamingTypes)
{
    auto module = createModule();

    DictPtr<IString, IStreamingType> streamingTypes;
    ASSERT_NO_THROW(streamingTypes = module.getAvailableStreamingTypes());
    ASSERT_EQ(streamingTypes.getCount(), 1u);
    ASSERT_TRUE(streamingTypes.hasKey("CredentialDemoStreaming"));
}

TEST_F(CredentialDemoModuleTest, CreateDeviceConnectionStringNull)
{
    auto module = createModule();

    DevicePtr device;
    ASSERT_THROW(device = module.createDevice(nullptr, nullptr), ArgumentNullException);
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceSucceedsAnonymously)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder().setAuthenticationMethod(StandardAnonymousAuthenticationMethod()).build();

    DevicePtr device;
    ASSERT_NO_THROW(device = module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters));
    ASSERT_TRUE(device.assigned());
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceSucceedsWithCorrectUserNamePassword)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"UserName", "user"}, {"Password", "pass"}}))
                                      .build();

    DevicePtr device;
    ASSERT_NO_THROW(device = module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters));
    ASSERT_TRUE(device.assigned());
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceThrowsWithWrongPassword)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"UserName", "user"}, {"Password", "wrong"}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters), AuthenticationFailedException);
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceThrowsWithWrongUserName)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"UserName", "wrong"}, {"Password", "pass"}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters), AuthenticationFailedException);
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceSucceedsWithCorrectPin)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPinAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"Pin", "1234"}}))
                                      .build();

    DevicePtr device;
    ASSERT_NO_THROW(device = module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters));
    ASSERT_TRUE(device.assigned());
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceThrowsWithWrongPin)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPinAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"Pin", "0000"}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters), AuthenticationFailedException);
}

#ifdef DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceSucceedsWithValidPrivateKey)
{
    const CredentialDemoTestTempFile publicKeyFile("credential_demo_module_test_public_key.pem", MatchingPublicKeyPem);
    const CredentialDemoTestTempFile privateKeyFile("credential_demo_module_test_private_key.pem", MatchingPrivateKeyPem);

    auto module = createModule(contextWithModuleOptions(Dict<IString, IBaseObject>({{"PublicKeyPath", publicKeyFile.path}})));

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPrivateKeyFileAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"PrivateKeyFilePath", privateKeyFile.path}}))
                                      .build();

    DevicePtr device;
    ASSERT_NO_THROW(device = module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters));
    ASSERT_TRUE(device.assigned());
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceThrowsWithMismatchedPrivateKey)
{
    const CredentialDemoTestTempFile publicKeyFile("credential_demo_module_test_public_key.pem", MatchingPublicKeyPem);
    const CredentialDemoTestTempFile privateKeyFile("credential_demo_module_test_wrong_private_key.pem", MismatchedPrivateKeyPem);

    auto module = createModule(contextWithModuleOptions(Dict<IString, IBaseObject>({{"PublicKeyPath", publicKeyFile.path}})));

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPrivateKeyFileAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"PrivateKeyFilePath", privateKeyFile.path}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters), AuthenticationFailedException);
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceThrowsWithNonexistentPrivateKeyFile)
{
    const CredentialDemoTestTempFile publicKeyFile("credential_demo_module_test_public_key.pem", MatchingPublicKeyPem);

    auto module = createModule(contextWithModuleOptions(Dict<IString, IBaseObject>({{"PublicKeyPath", publicKeyFile.path}})));

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPrivateKeyFileAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"PrivateKeyFilePath", "/no/such/private_key.pem"}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters), AuthenticationFailedException);
}

TEST_F(CredentialDemoModuleTest, AuthenticatedDeviceThrowsWhenModuleHasNoPublicKeyConfigured)
{
    const CredentialDemoTestTempFile privateKeyFile("credential_demo_module_test_private_key.pem", MatchingPrivateKeyPem);

    auto module = createModule(); // No "PublicKeyPath" module option set.

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPrivateKeyFileAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"PrivateKeyFilePath", privateKeyFile.path}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedDevice(DeviceConnectionString, nullptr, nullptr, loginParameters), AuthenticationFailedException);
}

#endif // DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH

TEST_F(CredentialDemoModuleTest, AuthenticatedStreamingSucceedsAnonymously)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder().setAuthenticationMethod(StandardAnonymousAuthenticationMethod()).build();

    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = module.createAuthenticatedStreaming(StreamingConnectionString, nullptr, loginParameters));
    ASSERT_TRUE(streaming.assigned());
}

TEST_F(CredentialDemoModuleTest, AuthenticatedStreamingSucceedsWithCorrectUserNamePassword)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"UserName", "user"}, {"Password", "pass"}}))
                                      .build();

    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = module.createAuthenticatedStreaming(StreamingConnectionString, nullptr, loginParameters));
    ASSERT_TRUE(streaming.assigned());
}

TEST_F(CredentialDemoModuleTest, AuthenticatedStreamingThrowsWithWrongPassword)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"UserName", "user"}, {"Password", "wrong"}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedStreaming(StreamingConnectionString, nullptr, loginParameters), AuthenticationFailedException);
}

TEST_F(CredentialDemoModuleTest, AuthenticatedStreamingSucceedsWithCorrectPin)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPinAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"Pin", "1234"}}))
                                      .build();

    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = module.createAuthenticatedStreaming(StreamingConnectionString, nullptr, loginParameters));
    ASSERT_TRUE(streaming.assigned());
}

TEST_F(CredentialDemoModuleTest, AuthenticatedStreamingThrowsWithWrongPin)
{
    auto module = createModule();

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPinAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"Pin", "0000"}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedStreaming(StreamingConnectionString, nullptr, loginParameters), AuthenticationFailedException);
}

#ifdef DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH

TEST_F(CredentialDemoModuleTest, AuthenticatedStreamingSucceedsWithValidPrivateKey)
{
    const CredentialDemoTestTempFile publicKeyFile("credential_demo_module_test_public_key.pem", MatchingPublicKeyPem);
    const CredentialDemoTestTempFile privateKeyFile("credential_demo_module_test_private_key.pem", MatchingPrivateKeyPem);

    auto module = createModule(contextWithModuleOptions(Dict<IString, IBaseObject>({{"PublicKeyPath", publicKeyFile.path}})));

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPrivateKeyFileAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"PrivateKeyFilePath", privateKeyFile.path}}))
                                      .build();

    StreamingPtr streaming;
    ASSERT_NO_THROW(streaming = module.createAuthenticatedStreaming(StreamingConnectionString, nullptr, loginParameters));
    ASSERT_TRUE(streaming.assigned());
}

TEST_F(CredentialDemoModuleTest, AuthenticatedStreamingThrowsWithMismatchedPrivateKey)
{
    const CredentialDemoTestTempFile publicKeyFile("credential_demo_module_test_public_key.pem", MatchingPublicKeyPem);
    const CredentialDemoTestTempFile privateKeyFile("credential_demo_module_test_wrong_private_key.pem", MismatchedPrivateKeyPem);

    auto module = createModule(contextWithModuleOptions(Dict<IString, IBaseObject>({{"PublicKeyPath", publicKeyFile.path}})));

    const auto loginParameters = LoginParametersBuilder()
                                      .setAuthenticationMethod(StandardPrivateKeyFileAuthenticationMethod())
                                      .setCredentials(Dict<IString, IString>({{"PrivateKeyFilePath", privateKeyFile.path}}))
                                      .build();

    ASSERT_THROW(module.createAuthenticatedStreaming(StreamingConnectionString, nullptr, loginParameters), AuthenticationFailedException);
}

#endif // DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH
