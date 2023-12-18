#include <opendaq/component_deserialize_context_impl.h>
#include <coretypes/validation.h>

namespace daq
{

ErrCode ComponentDeserializeContextImpl::getParent(IComponent** parent)
{
    OPENDAQ_PARAM_NOT_NULL(parent);

    *parent = this->parent.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ComponentDeserializeContextImpl::getLocalId(IString** localId)
{
    OPENDAQ_PARAM_NOT_NULL(localId);

    *localId = this->localId.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ComponentDeserializeContextImpl::getContext(IContext** context)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    *context = this->context.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

ErrCode ComponentDeserializeContextImpl::getTypeManager(ITypeManager** typeManager)
{
    OPENDAQ_PARAM_NOT_NULL(typeManager);

    *typeManager = this->typeManager.addRefAndReturn();

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(
    LIBRARY_FACTORY, ComponentDeserializeContext, IContext*, context, IComponent*, parent, IString*, localId, ITypeManager*, typeManager);

}
