#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/container_utils.h>
#include <opendaq/add_config_transport_selection.h>
#include <coreobjects/property_factory.h>
#include <opendaq/component_type_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_streaming_config
{
static const char* TypeConfig = "TypeConfig";
static const char* AuthenticationConfig = "AuthenticationConfig";

static const char* StreamingTypeKey = "streamingType";
static const char* SelectedTransportOptionKey = "selectedTransportOption";
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType)
    : AddStreamingConfigImpl(streamingType, DictPtr<IString, ITransportOption>())
{
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType,
                                               const DictPtr<IString, ITransportOption>& transportOptions)
    : AddStreamingConfigImpl(streamingType, Partial())
{
    Super::addProperty(ObjectProperty(add_streaming_config::TypeConfig, streamingType.createDefaultConfig()));
    if (this->streamingType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() > 0)
        Super::addProperty(ObjectProperty(add_streaming_config::AuthenticationConfig, streamingType.createAuthenticationConfig()));

    add_config::AddTransportSelectionProperties(objPtr,
                                                transportOptions.assigned() ? transportOptions : streamingType.getAvailableTransportOptions());
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType, Partial)
    : Super()
    , streamingType(streamingType)
{
}

ErrCode AddStreamingConfigImpl::getStreamingType(IStreamingType** type)
{
    OPENDAQ_PARAM_NOT_NULL(type);

    *type = streamingType.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddStreamingConfigImpl::getTypeConfig(IPropertyObject** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return daqTry([&] { *config = objPtr.getPropertyValue(add_streaming_config::TypeConfig).asPtr<IPropertyObject>().detach(); });
}

ErrCode AddStreamingConfigImpl::getAuthenticationConfig(IAuthenticationConfig** config)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    if (this->streamingType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "This streaming type supports no authentication method");

    return daqTry([&] { *config = objPtr.getPropertyValue(add_streaming_config::AuthenticationConfig).asPtr<IAuthenticationConfig>().detach(); });
}

ErrCode AddStreamingConfigImpl::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        auto lock = getRecursiveConfigLock2();
        auto obj = createWithImplementation<IPropertyObject, AddStreamingConfigImpl>(streamingType, Partial());
        dynamic_cast<AddStreamingConfigImpl*>(obj.getObject())->configureClonedMembers(getCloneParameters());
        add_config::FreezeAvailableTransportOptions(obj);
        *cloned = obj.detach();
    });
}

ErrCode AddStreamingConfigImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ErrCode AddStreamingConfigImpl::getAvailableTransportOptions(IDict** transportOptions)
{
    return add_config::GetAvailableTransportOptions(objPtr, transportOptions);
}

ErrCode AddStreamingConfigImpl::setTransportOption(IString* id)
{
    return add_config::SetTransportOption(objPtr, id);
}

ErrCode AddStreamingConfigImpl::getTransportOption(ITransportOption** transportOption)
{
    return add_config::GetTransportOption(objPtr, transportOption);
}

ErrCode AddStreamingConfigImpl::setPort(Int port)
{
    return add_config::SetPort(objPtr, port);
}

ErrCode AddStreamingConfigImpl::getPort(Int* port)
{
    return add_config::GetPort(objPtr, port);
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
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&]
    {
        const auto createConfig =
            [&factoryCallback](const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const StringPtr& /*className*/)
        {
            const StreamingTypePtr streamingType = serialized.readObject(add_streaming_config::StreamingTypeKey, context, factoryCallback);
            return createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>(streamingType, Partial());
        };

        *obj = Super::DeserializePropertyObject(serialized, context, factoryCallback, createConfig).detach();
    });
}

ErrCode AddStreamingConfigImpl::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    OPENDAQ_RETURN_IF_FAILED(Super::serializeCustomValues(serializer, forUpdate));

    return daqTry([&]
    {
        const auto serializerPtr = SerializerPtr::Borrow(serializer);
        if (forUpdate)
        {
            // The base class does not save a value equal to the default, and the default is the first option of this config.
            // A load creates the config from the type, whose first option may be another one.
            // So the selection is always saved, under its own key.
            if (add_config::HasTransportOptions(objPtr))
            {
                const StringPtr selected = objPtr.getPropertyValue(TransportOptionConfigProperty);
                serializerPtr.key(add_streaming_config::SelectedTransportOptionKey);
                serializerPtr.writeString(selected);
            }

            return;
        }

        serializerPtr.key(add_streaming_config::StreamingTypeKey);
        streamingType.asPtr<ISerializable>().serialize(serializerPtr);
    });
}

ErrCode AddStreamingConfigImpl::serializePropertyValue(const StringPtr& name,
                                                       const ObjectPtr<IBaseObject>& value,
                                                       ISerializer* serializer,
                                                       bool forUpdate)
{
    // The saved form leaves out two properties:
    // - AvailableTransportOptions: a load takes the options from the type, a bind from the source config;
    // - TransportOption: the selection is saved under its own key (see serializeCustomValues).
    if (forUpdate && (name == add_config::AvailableTransportOptionsProperty || name == TransportOptionConfigProperty))
        return OPENDAQ_SUCCESS;

    return Super::serializePropertyValue(name, value, serializer, forUpdate);
}

ErrCode AddStreamingConfigImpl::updateInternal(ISerializedObject* obj, IBaseObject* context)
{
    const ErrCode errCode = Super::updateInternal(obj, context);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    if (errCode == OPENDAQ_IGNORED)
        return errCode;

    return daqTry([&]
    {
        const auto saved = SerializedObjectPtr::Borrow(obj);
        // A selection that the loaded config does not list fails the load, also for a type without options.
        // The streaming must not change its transport silently.
        if (saved.hasKey(add_streaming_config::SelectedTransportOptionKey))
            checkErrorInfo(setTransportOption(saved.readString(add_streaming_config::SelectedTransportOptionKey)));
    });
}

#if !defined(BUILDING_STATIC_LIBRARY)

extern "C" ErrCode PUBLIC_EXPORT createAddStreamingConfig(IAddStreamingConfig** objTmp, IStreamingType* streamingType)
{
    return daq::createObject<IAddStreamingConfig, AddStreamingConfigImpl>(objTmp, streamingType);
}

#endif

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddStreamingConfigImpl)

END_NAMESPACE_OPENDAQ
