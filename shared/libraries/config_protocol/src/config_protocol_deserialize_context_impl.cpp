#include <config_protocol/config_protocol_deserialize_context_impl.h>

namespace daq::config_protocol
{

ConfigProtocolDeserializeContextImpl::ConfigProtocolDeserializeContextImpl(const ConfigProtocolClientCommPtr& clientComm,
                                                                           const std::string& remoteGlobalId,
                                                                           const ContextPtr& context,
                                                                           const ComponentPtr& parent,
                                                                           const StringPtr& localId)
    : GenericComponentDeserializeContextImpl(context, parent, localId)
    , clientComm(clientComm)
    , remoteGlobalId(remoteGlobalId)
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

ErrCode ConfigProtocolDeserializeContextImpl::clone(IComponent* newParent,
    IString* newLocalId,
    IComponentDeserializeContext** newComponentDeserializeContext)
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
        newParent,
        newLocalId);
}

}
