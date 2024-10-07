#include <config_protocol/config_protocol_streaming_consumer.h>
#include <opendaq/custom_log.h>
#include <config_protocol/config_mirrored_ext_sig_impl.h>
#include <opendaq/component_deserialize_context_factory.h>

namespace daq::config_protocol
{

ConfigProtocolStreamingConsumer::ConfigProtocolStreamingConsumer(const ContextPtr& daqContext, const FolderConfigPtr& externalSignalsFolder)
    : daqContext(daqContext)
    , loggerComponent(this->daqContext.getLogger().getOrAddComponent("ClientToServerStreamingConsumer"))
    , externalSignalsFolder(externalSignalsFolder)
{
}

ConfigProtocolStreamingConsumer::~ConfigProtocolStreamingConsumer()
{
    // remove all external signals added & connected via current session
    if (externalSignalsFolder.assigned() && !externalSignalsFolder.isRemoved())
    {
        for (const auto& [_, signal] : mirroredExternalSignals)
            externalSignalsFolder.removeItem(signal);
    }
    mirroredExternalSignals.clear();
    mirroredExternalSignalsIds.clear();
}

bool ConfigProtocolStreamingConsumer::isExternalSignal(const SignalPtr& signal)
{
    return signal.supportsInterface<IMirroredExternalSignalPrivate>();
}

void ConfigProtocolStreamingConsumer::processClientToServerStreamingPacket(SignalNumericIdType signalNumericId, const PacketPtr& packet)
{
    MirroredSignalConfigPtr signal;
    {
        std::scoped_lock lock(sync);
        if (const auto iter = mirroredExternalSignals.find(signalNumericId); iter != mirroredExternalSignals.end())
            signal = iter->second;
    }

    if (signal.assigned())
    {
        Bool forwardPacket = True;
        if (const auto eventPacket = packet.asPtrOrNull<IEventPacket>(); eventPacket.assigned())
            forwardPacket = signal.template asPtr<IMirroredSignalPrivate>().triggerEvent(eventPacket);
        if (forwardPacket)
            signal.sendPacket(packet);
    }
}

MirroredSignalConfigPtr ConfigProtocolStreamingConsumer::createMirroredExternalSignal(const StringPtr& signalStringId,
                                                                                      const StringPtr& serializedSignal,
                                                                                      SignalNumericIdType signalNumericId)
{
    const auto deserializer = JsonDeserializer();
    const auto deserializeContext = ComponentDeserializeContext(daqContext, nullptr, externalSignalsFolder, signalStringId);

    MirroredSignalConfigPtr signal = deserializer.deserialize(
        serializedSignal,
        deserializeContext,
        [](const StringPtr& typeId, const SerializedObjectPtr& object, const BaseObjectPtr& context, const FunctionPtr& factoryCallback) -> BaseObjectPtr
        {
            if (typeId != "Signal")
                return nullptr;
            BaseObjectPtr obj;
            checkErrorInfo(ConfigMirroredExternalSignalImpl::Deserialize(object, context, factoryCallback, &obj));
            return obj;
        });

    signal.asPtr<IComponentPrivate>().lockAllAttributes();
    signal.freeze();

    return signal;
}

bool ConfigProtocolStreamingConsumer::isForwardedCoreEvent(const ComponentPtr& component, const CoreEventArgsPtr& eventArgs)
{
    const auto coreEventId = static_cast<CoreEventId>(eventArgs.getEventId());

    if (component.assigned())
    {
        const auto isSignalOfConnectedClient = [this](const StringPtr& signalId)
        {
            std::scoped_lock lock(sync);
            const auto it = mirroredExternalSignalsIds.find(signalId);
            return it != mirroredExternalSignalsIds.end();
        };

        if (externalSignalsFolder.assigned())
        {
            if (component == externalSignalsFolder)
            {
                if (coreEventId == CoreEventId::ComponentAdded)
                {
                    StringPtr addedSignalId = eventArgs.getParameters().get("Component").asPtr<IComponent>().getLocalId();
                    return !isSignalOfConnectedClient(addedSignalId);
                }
                else if (coreEventId == CoreEventId::ComponentRemoved)
                {
                    StringPtr removedSignalId = eventArgs.getParameters().get("Id");
                    return !isSignalOfConnectedClient(removedSignalId);
                }
                else if (component.getParent() == externalSignalsFolder)
                {
                    return !isSignalOfConnectedClient(component.getLocalId());
                }
            }
        }
        else if (auto signal = component.asPtrOrNull<ISignal>(); signal.assigned() && isExternalSignal(signal))
        {
            return !isSignalOfConnectedClient(component.getLocalId());
        }

        if (coreEventId == CoreEventId::SignalConnected)
        {
            const SignalPtr connectedSignal = eventArgs.getParameters().get("Signal");
            if (connectedSignal.supportsInterface<IMirroredExternalSignalPrivate>())
                return !isSignalOfConnectedClient(connectedSignal.getLocalId());
        }
    }

    return true;
}

void ConfigProtocolStreamingConsumer::removeExternalSignals(const ParamsDictPtr& params)
{
    ListPtr<IInteger> signalNumericIdsList = params.get("SignalNumericIds");
    for (const auto& listItem : signalNumericIdsList)
    {
        SignalNumericIdType signalNumericId = static_cast<SignalNumericIdType>(listItem);
        if (const auto it = mirroredExternalSignals.find(signalNumericId); it != mirroredExternalSignals.end())
        {
            auto signal = it->second;
            removeExternalSignal(signal, signalNumericId);
        }
    }
}

void ConfigProtocolStreamingConsumer::addExternalSignal(const MirroredSignalConfigPtr& signal, SignalNumericIdType signalNumericId)
{
    {
        std::scoped_lock lock(sync);

        mirroredExternalSignalsIds.insert(signal.getLocalId());
        mirroredExternalSignals.insert({signalNumericId, signal});
    }

    if (externalSignalsFolder.assigned())
        externalSignalsFolder.addItem(signal);

    LOG_D("Added new mirrored external signal numeric ID {}, remote ID {}, global ID {}",
          signalNumericId,
          signal.getRemoteId(),
          signal.getGlobalId());
}

void ConfigProtocolStreamingConsumer::removeExternalSignal(const MirroredSignalConfigPtr& signal, SignalNumericIdType signalNumericId)
{
    auto signalLocalId = signal.getLocalId();
    LOG_D("Remove mirrored external signal numeric ID {}, remote ID {}, global ID {}",
          signalNumericId,
          signal.getRemoteId(),
          signal.getGlobalId());

    if (externalSignalsFolder.assigned())
        externalSignalsFolder.removeItem(signal);

    std::scoped_lock lock(sync);
    mirroredExternalSignals.erase(signalNumericId);
    mirroredExternalSignalsIds.erase(signalLocalId);
}

MirroredSignalConfigPtr ConfigProtocolStreamingConsumer::getOrAddExternalSignal(const ParamsDictPtr& params)
{
    const SignalNumericIdType domainSignalNumericId = params.get("DomainSignalNumericId");
    const StringPtr domainSignalStringId = params.get("DomainSignalStringId");
    const StringPtr domainSerializedSignal = params.get("DomainSerializedSignal");
    const SignalNumericIdType signalNumericId = params.get("SignalNumericId");
    const StringPtr signalStringId = params.get("SignalStringId");
    const StringPtr serializedSignal = params.get("SerializedSignal");

    MirroredSignalConfigPtr domainSignal;
    if (domainSignalNumericId != 0)
    {
        if (const auto iter = mirroredExternalSignals.find(domainSignalNumericId); iter == mirroredExternalSignals.end())
        {
            domainSignal = createMirroredExternalSignal(domainSignalStringId, domainSerializedSignal, domainSignalNumericId);
            addExternalSignal(domainSignal, domainSignalNumericId);
        }
        else
        {
            domainSignal = iter->second;
        }
    }

    MirroredSignalConfigPtr signal;
    if (const auto iter = mirroredExternalSignals.find(signalNumericId); iter == mirroredExternalSignals.end())
    {
        signal = createMirroredExternalSignal(signalStringId, serializedSignal, signalNumericId);
        if (domainSignal.assigned())
            signal.asPtr<IMirroredExternalSignalPrivate>()->assignDomainSignal(domainSignal);
        addExternalSignal(signal, signalNumericId);
    }
    else
    {
        signal = iter->second;
    }

    return signal;
}

}
