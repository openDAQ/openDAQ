#include <config_protocol/config_deserialize_context_impl.h>
#include <coretypes/validation.h>

namespace daq::config_protocol
{

ErrCode ConfigDeserializeContextImpl::getParent(IComponent** parent)
{
    OPENDAQ_PARAM_NOT_NULL(parent);

    *parent = this->parent.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ConfigDeserializeContextImpl::getLocalId(IString** localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    *localId = this->localId.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ConfigDeserializeContextImpl::getContext(IContext** context)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    *context = this->context.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ConfigDeserializeContextImpl::getTypeManager(ITypeManager** typeManager)
{
    OPENDAQ_PARAM_NOT_NULL(typeManager);

    *typeManager = this->typeManager.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

}
