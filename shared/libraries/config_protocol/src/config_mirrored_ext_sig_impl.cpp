#include <config_protocol/config_mirrored_ext_sig_impl.h>

namespace daq::config_protocol
{

ConfigMirroredExternalSignalImpl::ConfigMirroredExternalSignalImpl(const ContextPtr& ctx,
                                                                   const ComponentPtr& parent,
                                                                   const StringPtr& remoteGlobalId)
    : MirroredSignalBase(ctx, parent, createLocalId(remoteGlobalId), nullptr)
    , remoteGlobalId(remoteGlobalId)
{
    this->name = remoteGlobalId;
}

StringPtr ConfigMirroredExternalSignalImpl::onGetRemoteId() const
{
    return remoteGlobalId;
}

StringPtr ConfigMirroredExternalSignalImpl::createLocalId(const StringPtr& remoteGlobalId)
{
    static constexpr char delimeter = '*';
    std::string localId = remoteGlobalId;

    const char slash = '/';
    std::replace(localId.begin(), localId.end(), slash, delimeter);

    return String(localId);
}

Bool ConfigMirroredExternalSignalImpl::onTriggerEvent(const EventPacketPtr& eventPacket)
{
    return Super::onTriggerEvent(eventPacket);
}

ErrCode ConfigMirroredExternalSignalImpl::getGlobalId(IString** globalId)
{
    OPENDAQ_PARAM_NOT_NULL(globalId);

    *globalId = remoteGlobalId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode ConfigMirroredExternalSignalImpl::getLocalId(IString** localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    *localId = remoteGlobalId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

void ConfigMirroredExternalSignalImpl::assignDomainSignal(const SignalPtr& domainSignal)
{
    if (domainSignal.assigned())
        if (domainSignal.asPtrOrNull<IMirroredSignalConfig>() == nullptr)
        {
            throw NoInterfaceException(
                fmt::format(R"(Domain signal "{}" does not implement IMirroredSignalConfig interface.)",
                            domainSignal.getGlobalId()));
        }
    if (domainSignal.assigned())
        setMirroredDomainSignal(domainSignal.asPtr<IMirroredSignalConfig>());
    else
        setMirroredDomainSignal(nullptr);
}

ErrCode ConfigMirroredExternalSignalImpl::Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializeComponent(
                       serialized,
                       context,
                       factoryCallback,
                       [](const SerializedObjectPtr& serialized,
                          const ComponentDeserializeContextPtr& deserializeContext,
                          const StringPtr& className)
                       {
                           return createWithImplementation<ISignal, ConfigMirroredExternalSignalImpl>(
                               deserializeContext.getContext(), deserializeContext.getParent(), deserializeContext.getLocalId());
                       }).detach();
        });
}

void ConfigMirroredExternalSignalImpl::deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                                             const BaseObjectPtr& context,
                                                             const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);
    checkErrorInfo(setMirroredDataDescriptor(this->dataDescriptor));
}

SignalPtr ConfigMirroredExternalSignalImpl::onGetDomainSignal()
{
    return mirroredDomainSignal.addRefAndReturn();
}

DataDescriptorPtr ConfigMirroredExternalSignalImpl::onGetDescriptor()
{
    return mirroredDataDescriptor.addRefAndReturn();
}

}
