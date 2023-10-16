#include <coreobjects/property_value_event_args_impl.h>

BEGIN_NAMESPACE_OPENDAQ

ErrCode PropertyValueEventArgsImpl::getProperty(IProperty** prop)
{
    if (prop == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return property by a null pointer.");
    }

    *prop = property.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyValueEventArgsImpl::getValue(IBaseObject** value)
{
    if (value == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Cannot return the value by a null pointer");
    }

    *value = newValue.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyValueEventArgsImpl::setValue(IBaseObject* value)
{
    newValue = value;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyValueEventArgsImpl::getPropertyEventType(PropertyEventType* changeType)
{
    if (changeType == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *changeType = type;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyValueEventArgs, IProperty*, prop, IBaseObject*, value, PropertyEventType, type)

END_NAMESPACE_OPENDAQ
