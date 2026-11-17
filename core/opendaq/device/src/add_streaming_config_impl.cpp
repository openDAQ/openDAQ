#include <opendaq/add_streaming_config_impl.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_factory.h>
#include <opendaq/component_type_private_ptr.h>
#include <coreobjects/eval_value_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace add_streaming_config
{
static const char* TypeConfig = "TypeConfig";
static const char* AuthenticationConfig = "AuthenticationConfig";
static const char* TransportOption = "TransportOption";
static const char* Port = "Port";

static const char* StreamingTypeKey = "streamingType";
static const char* TransportOptionsKey = "transportOptions";
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType)
    : AddStreamingConfigImpl(streamingType, ListPtr<ITransportOption>())
{
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType, const ListPtr<ITransportOption>& transportOptions)
    : AddStreamingConfigImpl(streamingType, transportOptions.assigned() ? transportOptions : ComponentTypeTransportOptions(streamingType), Partial())
{
    Super::addProperty(ObjectProperty(add_streaming_config::TypeConfig, streamingType.createDefaultConfig()));
    // a type without authentication methods has no authentication config (createAuthenticationConfig fails for it)
    if (streamingType.asPtr<IComponentTypePrivate, ComponentTypePrivatePtr>().getSupportedAuthenticationMethods().getCount() > 0)
        Super::addProperty(ObjectProperty(add_streaming_config::AuthenticationConfig, streamingType.createAuthenticationConfig()));

    // The selected transport option, and the port to connect at, which is the one of the selected option until it is set
    if (this->transportOptions.getCount() > 0)
    {
        const TransportOptionPtr firstOption = this->transportOptions[0];
        Super::addProperty(StringPropertyBuilder(add_streaming_config::TransportOption, firstOption.getName())
                               .setSelectionValues(TransportOptionNames(this->transportOptions))
                               .build());
        Super::addProperty(IntProperty(
            add_streaming_config::Port, EvalValue(SelectedTransportOptionPortEval(this->transportOptions, add_streaming_config::TransportOption))));
    }
}

AddStreamingConfigImpl::AddStreamingConfigImpl(const StreamingTypePtr& streamingType, const ListPtr<ITransportOption>& transportOptions, Partial)
    : Super()
    , streamingType(streamingType)
    , transportOptions(transportOptions)
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

    if (!objPtr.hasProperty(add_streaming_config::AuthenticationConfig))
    {
        *config = nullptr;
        return OPENDAQ_SUCCESS;
    }

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

ErrCode AddStreamingConfigImpl::getTransportOptions(IList** transportOptions)
{
    OPENDAQ_PARAM_NOT_NULL(transportOptions);

    *transportOptions = this->transportOptions.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode AddStreamingConfigImpl::setTransportOption(IString* name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    if (transportOptions.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The Streaming type has no transport options");

    return daqTry([&]
    {
        const StringPtr nameStr = name;
        if (!FindTransportOption(transportOptions, nameStr).assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(There is no transport option "{}" to select)", nameStr));

        auto lock = getRecursiveConfigLock2();
        checkErrorInfo(Super::setPropertyValue(String(add_streaming_config::TransportOption), nameStr));

        // The port of the newly selected option applies
        checkErrorInfo(Super::clearPropertyValue(String(add_streaming_config::Port)));
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AddStreamingConfigImpl::getSelectedTransportOption(ITransportOption** transportOption)
{
    OPENDAQ_PARAM_NOT_NULL(transportOption);

    if (transportOptions.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The Streaming type has no transport options");

    return daqTry([&]
    {
        const StringPtr selected = objPtr.getPropertyValue(add_streaming_config::TransportOption);
        TransportOptionPtr option = FindTransportOption(transportOptions, selected);
        if (!option.assigned())
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(The selected transport option "{}" is not one of the options)", selected));

        *transportOption = option.detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode AddStreamingConfigImpl::setPort(IInteger* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    if (transportOptions.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The Streaming type has no transport options");

    if (static_cast<Int>(IntegerPtr(port)) < 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "The port must not be negative");

    return Super::setPropertyValue(String(add_streaming_config::Port), port);
}

ErrCode AddStreamingConfigImpl::getPort(IInteger** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    if (transportOptions.getCount() == 0)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "The Streaming type has no transport options");

    return daqTry([&] { *port = objPtr.getPropertyValue(add_streaming_config::Port).asPtr<IInteger>().detach(); });
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
            const ListPtr<ITransportOption> options =
                serialized.hasKey(add_streaming_config::TransportOptionsKey)
                    ? TransportOptionsFromPropertyObjects(serialized.readObject(add_streaming_config::TransportOptionsKey, context, factoryCallback))
                    : ComponentTypeTransportOptions(streamingType);

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
            serializerPtr.key(add_streaming_config::TransportOptionsKey);
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
