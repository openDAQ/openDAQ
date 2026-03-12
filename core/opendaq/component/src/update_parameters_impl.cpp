#include <opendaq/update_parameters_impl.h>
BEGIN_NAMESPACE_OPENDAQ

UpdateParametersImpl::UpdateParametersImpl()
    : Super()
{
    Super::addProperty(BoolProperty("RemoteUpdate", false));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr UpdateParametersImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name);
}

ErrCode UpdateParametersImpl::getDeviceUpdateOptions(IDeviceUpdateOptions** options)
{
    OPENDAQ_PARAM_NOT_NULL(options);

    *options = deviceOptions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UpdateParametersImpl::setDeviceUpdateOptions(IDeviceUpdateOptions* options)
{
    deviceOptions = options;
    return OPENDAQ_SUCCESS;
}

ErrCode UpdateParametersImpl::serializeCustomValues(ISerializer* serializer, bool)
{
    if (deviceOptions.assigned())
    {
        serializer->key("DeviceUpdateOptions");
        OPENDAQ_RETURN_IF_FAILED(deviceOptions.asPtr<ISerializable>()->serialize(serializer), "Failed to serialize DeviceUpdateOptions");
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, UpdateParameters)

END_NAMESPACE_OPENDAQ
