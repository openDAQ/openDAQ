#include <coreobjects/end_update_event_args_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

EndUpdateEventArgsImpl::EndUpdateEventArgsImpl(const ListPtr<IString>& properties)
    : EventArgsImplTemplate<IEndUpdateEventArgs>(0, "EndUpdateEvent")
    , properties(properties)
{
}

ErrCode EndUpdateEventArgsImpl::getProperties(IList** properties)
{
    OPENDAQ_PARAM_NOT_NULL(properties);

    *properties = this->properties.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, EndUpdateEventArgs, IList*, properties)

END_NAMESPACE_OPENDAQ
