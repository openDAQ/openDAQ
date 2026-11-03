#include <opendaq/add_device_config_impl.h>

BEGIN_NAMESPACE_OPENDAQ

AddDeviceConfigImpl::AddDeviceConfigImpl(const ListPtr<IString>& transports)
    : Super(transports)
{
}

ErrCode AddDeviceConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    auto obj = createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>();

    return daqTry([this, &obj, &cloned]
    {
        auto implPtr = dynamic_cast<AddDeviceConfigImpl*>(obj.getObject());
        implPtr->configureClonedMembers(getCloneParameters());

        *cloned = obj.detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AddDeviceConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr AddDeviceConfigImpl::SerializeId()
{
    return "AddDeviceConfig";
}

ErrCode AddDeviceConfigImpl::Deserialize(ISerializedObject* serialized,
                                         IBaseObject* context,
                                         IFunction* factoryCallback,
                                         IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        // The "Transport" selection property comes back with the serialized local properties
        *obj = Super::DeserializePropertyObject(
                   serialized,
                   context,
                   factoryCallback,
                   [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
                   {
                       return createWithImplementation<IAddDeviceConfig, AddDeviceConfigImpl>();
                   })
                   .detach();
    });
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddDeviceConfigImpl)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AddDeviceConfig, IAddDeviceConfig,
    IList*, transports
)

END_NAMESPACE_OPENDAQ
