#include <config_protocol/config_protocol_deserialize_context_impl.h>

namespace daq::config_protocol
{

ConfigProtocolDeserializeContextImpl::ConfigProtocolDeserializeContextImpl(const ConfigProtocolClientCommPtr& clientComm,
                                                                           const std::string& remoteGlobalId,
                                                                           const ContextPtr& context,
                                                                           const ComponentPtr& root,
                                                                           const ComponentPtr& parent,
                                                                           const StringPtr& localId,
                                                                           IntfID* inftID,
                                                                           const ProcedurePtr& triggerCoreEvent,
                                                                           const TypeManagerPtr& typeManager)
    : GenericComponentDeserializeContextImpl(context.assigned() ? context : clientComm->getDaqContext(), root, parent, localId, inftID, triggerCoreEvent)
    , clientComm(clientComm)
    , remoteGlobalId(remoteGlobalId)
    , typeManager(typeManager)
{
}

ConfigProtocolClientCommPtr ConfigProtocolDeserializeContextImpl::getClientComm()
{
    return clientComm;
}

std::string ConfigProtocolDeserializeContextImpl::getRemoteGlobalId()
{
    return remoteGlobalId;
}

void ConfigProtocolDeserializeContextImpl::setRemoteGlobalId(const std::string& remoteGlobalId)
{
    this->remoteGlobalId = remoteGlobalId;
}

TypeManagerPtr ConfigProtocolDeserializeContextImpl::getTypeManager()
{
    if (typeManager.assigned())
        return typeManager;
    if (context.assigned())
        return context.getTypeManager();
    return nullptr;
}

ErrCode ConfigProtocolDeserializeContextImpl::clone(IComponent* newParent,
                                                    IString* newLocalId,
                                                    IComponentDeserializeContext** newComponentDeserializeContext,
                                                    IntfID* newIntfID,
                                                    IProcedure* newTriggerCoreEvent)
{
    OPENDAQ_PARAM_NOT_NULL(newLocalId);
    OPENDAQ_PARAM_NOT_NULL(newComponentDeserializeContext);

    const auto newLocalIdPtr = StringPtr::Borrow(newLocalId);

    std::string newRemoteGlobalId = remoteGlobalId + "/" + newLocalIdPtr.toStdString();

    return createObject<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        newComponentDeserializeContext,
        clientComm,
        newRemoteGlobalId,
        this->context,
        this->root,
        newParent,
        newLocalId,
        newIntfID,
        newTriggerCoreEvent);
}

}
