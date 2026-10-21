#include <opendaq/add_streaming_config_impl.h>

BEGIN_NAMESPACE_OPENDAQ

AddStreamingConfigImpl::AddStreamingConfigImpl(const ListPtr<IString>& transports)
    : Super(transports)
{
}

ErrCode AddStreamingConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    auto obj = createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>();

    return daqTry([this, &obj, &cloned]
    {
        auto implPtr = dynamic_cast<AddStreamingConfigImpl*>(obj.getObject());
        implPtr->configureClonedMembers(getCloneParameters());

        *cloned = obj.detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AddStreamingConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr AddStreamingConfigImpl::SerializeId()
{
    return "AddStreamingConfig";
}

ErrCode AddStreamingConfigImpl::Deserialize(ISerializedObject* serialized,
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
                       return createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>();
                   })
                   .detach();
    });
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddStreamingConfigImpl)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, AddStreamingConfig, IAddStreamingConfig,
    IList*, transports
)

END_NAMESPACE_OPENDAQ
