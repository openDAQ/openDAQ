#include <config_protocol/config_mirrored_ext_sig_impl.h>

namespace daq::config_protocol
{

ConfigMirroredExternalSignalImpl::ConfigMirroredExternalSignalImpl(const ContextPtr& ctx,
                                                                   const ComponentPtr& parent,
                                                                   const StringPtr& remoteGlobalId)
    : MirroredSignalBase(ctx, parent, createLocalId(remoteGlobalId), nullptr)
    , remoteGlobalId(remoteGlobalId)
{
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
    if (!eventPacket.assigned())
        return False;

    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const auto params = eventPacket.getParameters();
        DataDescriptorPtr signalDescriptorParam = params[event_packet_param::DATA_DESCRIPTOR];
        DataDescriptorPtr domainDescriptorParam = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];
        const bool signalDescriptorChanged = signalDescriptorParam.assigned();
        const bool domainDescriptorChanged = domainDescriptorParam.assigned();
        const DataDescriptorPtr newSignalDescriptor = signalDescriptorParam != NullDataDescriptor() ? signalDescriptorParam : nullptr;
        const DataDescriptorPtr newDomainDescriptor = domainDescriptorParam != NullDataDescriptor() ? domainDescriptorParam : nullptr;

        Bool changed = False;

        {
            std::scoped_lock lock(signalMutex);
            if (signalDescriptorChanged && newSignalDescriptor != mirroredDataDescriptor)
            {
                mirroredDataDescriptor = newSignalDescriptor;
                changed = True;
            }
        }
        if (changed && !this->coreEventMuted && this->coreEvent.assigned())
        {
            const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                CoreEventId::DataDescriptorChanged, Dict<IString, IBaseObject>({{"DataDescriptor", newSignalDescriptor}}));

            this->triggerCoreEvent(args);
        }

        {
            std::scoped_lock lock(signalMutex);
            if (domainDescriptorChanged && mirroredDomainDataDescriptor != newDomainDescriptor)
            {
                mirroredDomainDataDescriptor = newDomainDescriptor;
                if (mirroredDomainSignal.assigned())
                {
                    const auto domainSignalEventPacket = DataDescriptorChangedEventPacket(newDomainDescriptor, nullptr);
                    mirroredDomainSignal.asPtr<IMirroredSignalPrivate>().triggerEvent(domainSignalEventPacket);
                }
                changed = True;
            }
        }

        return changed;
    }

    // packet was not handled so returns True to forward the original packet
    return True;
}

void ConfigMirroredExternalSignalImpl::assignDomainSignal(const MirroredSignalConfigPtr& domainSignal)
{
    if (domainSignal.assigned() && !domainSignal.supportsInterface<IMirroredExternalSignalPrivate>())
        throw NoInterfaceException(fmt::format(R"(Domain signal "{}" is not valid.)", domainSignal.getGlobalId()));

    if (mirroredDomainSignal == domainSignal)
        return;

    if (domainSignal.assigned())
        setMirroredDomainSignal(domainSignal);
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
