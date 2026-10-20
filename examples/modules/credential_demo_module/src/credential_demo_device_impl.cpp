#include <credential_demo_module/credential_demo_device_impl.h>
#include <credential_demo_module/credential_demo_streaming_impl.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/device_type_factory.h>
#include <opendaq/component_type_builder_factory.h>
#include <opendaq/credential_request_factory.h>
#include <opendaq/authentication_method_factory.h>
#include <opendaq/server_capability_config.h>
#include <opendaq/device_info_internal.h>
#include <opendaq/component_type_private.h>
#include <opendaq/streaming_ptr.h>
#include <coretypes/dictobject_factory.h>
#include <fmt/format.h>
#include <string_view>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static constexpr std::string_view GenericDeviceAddress = "credential_demo_device";

CredentialDemoDeviceImpl::CredentialDemoDeviceImpl(const PropertyObjectPtr& config,
                                                   const ContextPtr& ctx,
                                                   const ComponentPtr& parent,
                                                   const DeviceInfoPtr& info,
                                                   bool authenticated,
                                                   const StringPtr& authenticationMethodId,
                                                   const DictPtr<IString, IString>& credentials)
    : MirroredDevice(ctx, parent, fmt::format("{}_{}", info.getManufacturer(), info.getSerialNumber()), nullptr, info.getName())
{
    if (authenticated)
    {
        authentication::Authenticate(ctx, credentials, authenticationMethodId);
    }

    this->deviceInfo = info;
}

StringPtr CredentialDemoDeviceImpl::onGetRemoteId() const
{
    // No real remote counterpart exists for this in-process demo device - its own local id (the same
    // id it's incorporated into the local component tree under) doubles as its remote id.
    return localId;
}

bool CredentialDemoDeviceImpl::isAddedToLocalComponentTree()
{
    return true;
}

DeviceInfoPtr CredentialDemoDeviceImpl::CreateDeviceInfo(const DictPtr<IString, IBaseObject>& moduleOptions, const ContextPtr& context)
{
    const StringPtr manufacturer = moduleOptions.get("Manufacturer");
    const StringPtr serialNumber = moduleOptions.get("SerialNumber");

    auto connectionString = fmt::format("{}://{}", Prefix, GenericDeviceAddress);
    auto devInfo = DeviceInfo(connectionString);
    devInfo.setName("Credential demo device");
    devInfo.setManufacturer(manufacturer);
    devInfo.setModel("Credential demo device");
    devInfo.setSerialNumber(serialNumber);
    auto deviceType = CreateType(context);
    devInfo.setDeviceType(deviceType);

    auto capability = ServerCapability("CredentialDemo", "Credential Demo", ProtocolType::Configuration)
                           .setPrefix(Prefix)
                           .setConnectionString(connectionString);
    DictPtr<IString, IAuthenticationMethod> methods;
    checkErrorInfo(deviceType.asPtr<IComponentTypePrivate>()->getSupportedAuthenticationMethods(&methods));
    for (const auto& [id, method] : methods)
        capability.addAuthenticationMethod(id);
    devInfo.asPtr<IDeviceInfoInternal>(true).addServerCapability(capability);

    // The`addDevice`'s automatic streaming attach (`PrioritizedStreamingProtocols`)
    // can pick "CredentialDemoStreaming" up for this device without any manual `addStreaming` call.
    auto streamingConnectionString = fmt::format("{}://{}", CredentialDemoStreamingImpl::Prefix, GenericDeviceAddress);
    auto streamingType = CredentialDemoStreamingImpl::CreateType(context);
    auto streamingCapability = ServerCapability("CredentialDemoStreaming", "Credential Demo Streaming", ProtocolType::Streaming)
                                    .setPrefix(CredentialDemoStreamingImpl::Prefix)
                                    .setConnectionString(streamingConnectionString);
    methods = nullptr;
    checkErrorInfo(streamingType.asPtr<IComponentTypePrivate>()->getSupportedAuthenticationMethods(&methods));
    for (const auto& [id, method] : methods)
        streamingCapability.addAuthenticationMethod(id);
    devInfo.asPtr<IDeviceInfoInternal>(true).addServerCapability(streamingCapability);

    return devInfo;
}

DeviceTypePtr CredentialDemoDeviceImpl::CreateType(const ContextPtr& context)
{
    // Showcases UserName/Password, PIN, Anonymous, and (when the OpenSSL-backed
    // `DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH` build option is on) PrivateKeyFile
    // authentication - defaulting to Anonymous, the first one added.
    auto builder = DeviceTypeBuilder()
        .setId("CredentialDemoDevice")
        .setName("Credential demo device")
        .setDescription("openDAQ authentication/credential framework showcase device")
        .setConnectionStringPrefix(Prefix)
        .addAuthenticationMethod(StandardAnonymousAuthenticationMethod())
        .addAuthenticationMethod(StandardUserNamePasswordAuthenticationMethod())
        .addAuthenticationMethod(StandardPinAuthenticationMethod());

#ifdef DAQMODULES_CREDENTIAL_DEMO_MODULE_ENABLE_PRIVATE_KEY_AUTH
    builder.addAuthenticationMethod(StandardPrivateKeyFileAuthenticationMethod());
#endif

    return builder.build();
}

void CredentialDemoDeviceImpl::ValidateConnectionString(const StringPtr& connectionString)
{
    const std::string prefix = fmt::format("{}://", Prefix);
    const std::string connStr = connectionString;
    if (connStr.find(prefix) != 0)
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid connection string \"{}\", no prefix", connectionString);
    }

    const auto address = connStr.substr(prefix.size());
    if (address != GenericDeviceAddress)
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid connection string \"{}\", unknown device address", connectionString);
    }
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
