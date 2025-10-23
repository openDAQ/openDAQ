#include <level4/property_object_view_impl.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename Impl>
template <class... Args>
GenericPropertyObjectView<Impl>::GenericPropertyObjectView(IPropertyObject* viewOwner,
                                                           const Args&... args)
    : Impl(args...)
    , viewOwner(viewOwner)
{
}

template <typename Impl>
ErrCode INTERFACE_FUNC GenericPropertyObjectView<Impl>::getViewOwner(IPropertyObject** propObject)
{
    OPENDAQ_PARAM_NOT_NULL(propObject);

    auto owner = viewOwner.getRef();
    if (!owner.assigned())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE);

    *propObject = owner.detach();
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
