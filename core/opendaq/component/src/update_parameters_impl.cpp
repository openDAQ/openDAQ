#include <opendaq/update_parameters_impl.h>
BEGIN_NAMESPACE_OPENDAQ

UpdateParametersImpl::UpdateParametersImpl()
    : Super()
{
    Super::addProperty(BoolProperty("ReAddDevices", false));
}

template <typename T>
typename InterfaceToSmartPtr<T>::SmartPtr UpdateParametersImpl::getTypedProperty(const StringPtr& name)
{
    return objPtr.getPropertyValue(name);
}

ErrCode UpdateParametersImpl::getReAddDevicesEnabled(Bool* enabled)
{
    return daqTry([&]
    {
        *enabled = getTypedProperty<IBoolean>("ReAddDevices");
        return OPENDAQ_SUCCESS;
    });
}

ErrCode UpdateParametersImpl::setReAddDevicesEnabled(Bool enabled)
{
    return Super::setPropertyValue(String("ReAddDevices"), BooleanPtr(enabled));
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, UpdateParameters)

END_NAMESPACE_OPENDAQ