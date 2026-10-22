#include <credential_demo_module/credential_demo_streaming_impl.h>
#include <credential_demo_module/credential_demo_device_impl.h>

#include <opendaq/streaming_type_factory.h>

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

static const std::string CredentialDemoStreamingTypeId = "CredentialDemoStreaming";

CredentialDemoStreamingImpl::CredentialDemoStreamingImpl(const StringPtr& connectionString,
                                                          const ContextPtr& ctx,
                                                          const StringPtr& authenticationMethodId,
                                                          const DictPtr<IString, IString>& credentials)
    : Streaming(connectionString, ctx, /*skipDomainSignalSubscribe*/ true)
{
    authentication::Authenticate(ctx, credentials, authenticationMethodId);
}

StreamingTypePtr CredentialDemoStreamingImpl::CreateType(const ContextPtr& context)
{
    // Shares its connection parameters - including the device's four authentication methods - with the device
    // type, rather than declaring its own.
    return StreamingTypeBuilder()
        .setId(CredentialDemoStreamingTypeId)
        .setName("Credential demo streaming")
        .setDescription("Dummy streaming connection, authenticated via the same credential framework and "
                         "auth methods as the device")
        .setConnectionStringPrefix(Prefix)
        .setLinkedDeviceType(CredentialDemoDeviceImpl::CreateType(context))
        .build();
}

void CredentialDemoStreamingImpl::onSetActive(bool /*active*/)
{
}

void CredentialDemoStreamingImpl::onAddSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void CredentialDemoStreamingImpl::onRemoveSignal(const MirroredSignalConfigPtr& /*signal*/)
{
}

void CredentialDemoStreamingImpl::onSubscribeSignal(const StringPtr& /*signalStreamingId*/)
{
}

void CredentialDemoStreamingImpl::onUnsubscribeSignal(const StringPtr& /*signalStreamingId*/)
{
}

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
