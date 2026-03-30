#include <opendaq/update_parameters_impl.h>
#include <opendaq/update_parameters_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

UpdateParametersImpl::UpdateParametersImpl()
    : Super()
{
    Super::addProperty(BoolProperty("RemoteUpdate", true));
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

ErrCode UpdateParametersImpl::getRemoteUpdateEnabled(Bool* enabled)
{
    OPENDAQ_PARAM_NOT_NULL(enabled);

    return daqTry([&]
    {
        *enabled = getTypedProperty<IBoolean>("RemoteUpdate");
        return OPENDAQ_SUCCESS;
    });
}

ErrCode UpdateParametersImpl::setRemoteUpdateEnabled(Bool enabled)
{
    return Super::setPropertyValue(String("RemoteUpdate"), BooleanPtr(enabled));
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
ErrCode UpdateParametersImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr UpdateParametersImpl::SerializeId()
{
    return "UpdateParameters";
}

ErrCode UpdateParametersImpl::Deserialize(ISerializedObject* serialized,
                                          IBaseObject* context,
                                          IFunction* factoryCallback,
                                          IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    ErrCode err = daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            *obj = Super::DeserializePropertyObject(serialized, context, factoryCallback,
                   [](const SerializedObjectPtr& serialized, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                   {
                       const UpdateParametersPtr updateParameters = createWithImplementation<IUpdateParameters, UpdateParametersImpl>();
                       const auto serializedPtr = SerializedObjectPtr::Borrow(serialized);
                       if (serializedPtr.hasKey("DeviceUpdateOptions"))
                           updateParameters.setDeviceUpdateOptions(serializedPtr.readObject("DeviceUpdateOptions"));

                       return updateParameters;
                   }).detach();
        });

    OPENDAQ_RETURN_IF_FAILED(err);


    return OPENDAQ_SUCCESS;
}


OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, UpdateParameters)

END_NAMESPACE_OPENDAQ
