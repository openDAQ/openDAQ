#include <config_protocol/config_protocol_deserialize_context_impl.h>

namespace daq::config_protocol
{

ConfigProtocolDeserializeContextImpl::ConfigProtocolDeserializeContextImpl(const ConfigProtocolClientCommPtr& clientComm,
                                                                           const ContextPtr& context,
                                                                           const ComponentPtr& parent,
                                                                           const StringPtr& localId,
                                                                           const TypeManagerPtr& typeManager)
    : GenericComponentDeserializeContextImpl(context, parent, localId, typeManager)
    , clientComm(clientComm)
{
}

ConfigProtocolClientCommPtr ConfigProtocolDeserializeContextImpl::getClientComm()
{
    return clientComm;
}

ErrCode ConfigProtocolDeserializeContextImpl::clone(IComponent* newParent,
    IString* newLocalId,
    IComponentDeserializeContext** newComponentDeserializeContext)
{
    OPENDAQ_PARAM_NOT_NULL(newLocalId);
    OPENDAQ_PARAM_NOT_NULL(newComponentDeserializeContext);

    return createInterfaceWithImplementation<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
        newComponentDeserializeContext,
        clientComm,
        this->context,
        newParent,
        newLocalId,
        this->typeManager);
}
}
