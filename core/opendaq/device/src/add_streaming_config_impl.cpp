#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/transport_option_impl.h>
#include <opendaq/container_utils.h>
#include <coreobjects/property_factory.h>
#include <opendaq/component_type_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_streaming_config
{
static const char* TypeConfig = "TypeConfig";
static const char* AuthenticationConfig = "AuthenticationConfig";
static const char* TransportOption = "TransportOption";
static const char* Port = "Port";

static const char* StreamingTypeKey = "streamingType";
static const char* AvailableTransportOptionsKey = "availableTransportOptions";
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType)
    : AddStreamingConfigImpl(streamingType, DictPtr<IString, ITransportOption>())
{
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType,
                                               const DictPtr<IString, ITransportOption>& transportOptions)
    : AddStreamingConfigImpl(
          streamingType, transportOptions.assigned() ? transportOptions : streamingType.getAvailableTransportOptions(), Partial())
{
    Super::addProperty(ObjectProperty(add_streaming_config::TypeConfig, streamingType.createDefaultConfig()));
    if (this->streamingType.asPtr<IComponentTypePrivate>().getSupportedAuthenticationMethods().getCount() > 0)
        Super::addProperty(ObjectProperty(add_streaming_config::AuthenticationConfig, streamingType.createAuthenticationConfig()));

    // The selected transport option, for a type with options
    if (this->transportOptions.getCount() > 0)
    {
        const TransportOptionPtr firstOption = FirstValue(this->transportOptions);
        Super::addProperty(StringPropertyBuilder(add_streaming_config::TransportOption, firstOption.getId())
                               .setSelectionValues(this->transportOptions.getKeyList())
                               .build());
    }

    // The port set, -1 while none is set and the port of the selected option applies
    Super::addProperty(IntPropertyBuilder(add_streaming_config::Port, -1).setMinValue(-1).build());
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType,
                                               const DictPtr<IString, ITransportOption>& transportOptions,
                                               Partial)
    : Super()
    , streamingType(streamingType)
    , transportOptions(FrozenClone(transportOptions.assigned() ? transportOptions : Dict<IString, ITransportOption>()))
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
        auto obj = createWithImplementation<IPropertyObject, AddStreamingConfigImpl>(streamingType, transportOptions, Partial());
        dynamic_cast<AddStreamingConfigImpl*>(obj.getObject())->configureClonedMembers(getCloneParameters());
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
    OPENDAQ_PARAM_NOT_NULL(transportOptions);

    *transportOptions = this->transportOptions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddStreamingConfigImpl::setTransportOption(IString* id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    return daqTry([&]
    {
        const StringPtr idStr = id;
        if (!transportOptions.hasKey(idStr))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(There is no transport option "{}" to select)", idStr));

        // A port set stays: it overrides the port of whichever option is selected
        return Super::setPropertyValue(String(add_streaming_config::TransportOption), idStr);
    });
}

ErrCode AddStreamingConfigImpl::getSelectedTransportOption(ITransportOption** transportOption)
{
    OPENDAQ_PARAM_NOT_NULL(transportOption);

    if (transportOptions.getCount() == 0)
    {
        *transportOption = nullptr;
        return OPENDAQ_SUCCESS;
    }

    // The property only takes the ids of the options
    return daqTry([&]
    {
        const StringPtr selected = objPtr.getPropertyValue(add_streaming_config::TransportOption);
        *transportOption = transportOptions.get(selected).detach();
    });
}

ErrCode AddStreamingConfigImpl::setPort(Int port)
{
    // -1 drops the port set, so the port of the selected option applies again
    if (port == -1)
        return Super::clearPropertyValue(String(add_streaming_config::Port));

    if (port < 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The port must not be negative, other than -1");

    return Super::setPropertyValue(String(add_streaming_config::Port), Integer(port));
}

ErrCode AddStreamingConfigImpl::getPort(Int* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    return daqTry([&]
    {
        // A port set is used whichever option is selected; -1 means none is set
        const Int portSet = objPtr.getPropertyValue(add_streaming_config::Port);
        if (portSet != -1 || transportOptions.getCount() == 0)
        {
            *port = portSet;
            return;
        }

        const StringPtr selected = objPtr.getPropertyValue(add_streaming_config::TransportOption);
        *port = transportOptions.get(selected).getPort();
    });
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

            // A config serialized without options of its own lists the Streaming type's
            const DictPtr<IString, ITransportOption> options =
                serialized.hasKey(add_streaming_config::AvailableTransportOptionsKey)
                    ? DictPtr<IString, ITransportOption>(
                          serialized.readObject(add_streaming_config::AvailableTransportOptionsKey, context, factoryCallback).asPtr<IDict>())
                    : streamingType.getAvailableTransportOptions();

            return createWithImplementation<IAddStreamingConfig, AddStreamingConfigImpl>(streamingType, options, Partial());
        };

        *obj = Super::DeserializePropertyObject(serialized, context, factoryCallback, createConfig).detach();
    });
}

ErrCode AddStreamingConfigImpl::serializeCustomValues(ISerializer* serializer, bool forUpdate)
{
    OPENDAQ_RETURN_IF_FAILED(Super::serializeCustomValues(serializer, forUpdate));
    if (forUpdate)
        return OPENDAQ_SUCCESS;

    return daqTry([&]
    {
        const auto serializerPtr = SerializerPtr::Borrow(serializer);
        serializerPtr.key(add_streaming_config::StreamingTypeKey);
        streamingType.asPtr<ISerializable>().serialize(serializerPtr);

        // The options may be the ones the server of the device announces, which the Streaming type does not list
        if (transportOptions.getCount() > 0)
        {
            serializerPtr.key(add_streaming_config::AvailableTransportOptionsKey);
            transportOptions.serialize(serializerPtr);
        }
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
