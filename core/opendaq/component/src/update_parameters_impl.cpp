#include <opendaq/update_parameters_impl.h>
BEGIN_NAMESPACE_OPENDAQ

const char* ReaddDevices = "ReAddDevices";
const char* RemoteUpdate = "RemoteUpdate";

UpdateParametersImpl::UpdateParametersImpl()
    : Super()
{
    Super::addProperty(BoolProperty(ReaddDevices, false));
    Super::addProperty(BoolProperty(RemoteUpdate, true));
}

ConstCharPtr UpdateParametersImpl::SerializeId()
{
    return "UpdateParameters";
}

ErrCode UpdateParametersImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ErrCode UpdateParametersImpl::Deserialize(ISerializedObject* serialized,
                                          IBaseObject* context,
                                          IFunction* factoryCallback,
                                          IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = Super::DeserializePropertyObject(
            serialized,
            context,
            factoryCallback,
            [](const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr& className)
            {
                return createWithImplementation<IUpdateParameters, UpdateParametersImpl>();
            }).detach();
    });
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
        *enabled = getTypedProperty<IBoolean>(ReaddDevices);
    });
}

ErrCode UpdateParametersImpl::setReAddDevicesEnabled(Bool enabled)
{
    return Super::setPropertyValue(String(ReaddDevices), BooleanPtr(enabled));
}

ErrCode UpdateParametersImpl::getRemoteUpdate(Bool* remoteUpdate)
{
    return daqTry([&]
    {
        *remoteUpdate = getTypedProperty<IBoolean>(RemoteUpdate);
    });
}

ErrCode UpdateParametersImpl::setRemoteUpdate(Bool remoteUpdate)
{
    return Super::setPropertyValue(String(RemoteUpdate), BooleanPtr(remoteUpdate));
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, UpdateParameters)

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(UpdateParametersImpl)

END_NAMESPACE_OPENDAQ